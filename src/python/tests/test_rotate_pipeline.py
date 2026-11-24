"""Tests for the rotate stage of llsm-pipeline.py and mosaic-pipeline.py.

Config sanitizing is checked for both scripts. process() is exercised in dry-run mode on the MOSAIC script
with synthetic Settings files and empty placeholder images (only file names matter in a dry run).
"""
import contextlib
import importlib.util
import io
import json
import math
import re
import sys
import tempfile
import unittest
from pathlib import Path

PYTHON_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PYTHON_DIR))


def load_script(filename, module_name):
    spec = importlib.util.spec_from_file_location(module_name, PYTHON_DIR / filename)
    module = importlib.util.module_from_spec(spec)
    with contextlib.redirect_stdout(io.StringIO()):
        spec.loader.exec_module(module)
    return module


MOSAIC = load_script('mosaic-pipeline.py', 'mosaic_pipeline')
LLSM = load_script('llsm-pipeline.py', 'llsm_pipeline')

STAGE_STEP = 0.5
XY = 0.108
DESKEW_ANGLE = 147.55
EXPECTED_Z = abs(STAGE_STEP * math.sin(DESKEW_ANGLE * math.pi / 180.0))
EXPECTED_SHIFT = STAGE_STEP * math.cos(DESKEW_ANGLE * math.pi / 180.0) / XY  # negative for the MOSAIC


def mosaic_settings(objective_scanned, interval=None):
    """Minimal MOSAIC Settings.txt: two channels (514 and 560 nm)."""
    if objective_scanned:
        motion = 'Z galvo & DO XZ stage'
        offset = ''.join('XZ stage Offset, Interval (um), # of Pixels for Excitation (%d) :\t0\t%s\t401\n' % (i, interval or 0.2)
                         for i in range(2))
    else:
        motion = 'X stage'
        offset = ''.join('X Stage Offset, Interval (um), # of Pixels for Excitation (%d) :\t5\t%s\t401\n' % (i, interval or STAGE_STEP)
                         for i in range(2))
    return (
        '***** ***** ***** General ***** ***** ***** \nDate :\t10/30/2026\nAcq Mode :\tZ stack\nVersion :\tv 4.0\nPC :\tX\n\n'
        '***** ***** ***** Waveform ***** ***** ***** \nWaveform type :\tLinear\n\n'
        + offset +
        '\n# of stacks (0) :\t1\n# of stacks (1) :\t1\n\n'
        'Excitation Filter, Laser, Power (%, Exp(ms), Laser2, Power2 (%), Laser3, Power3 (%) (0) :\tN/A\t514\t4\t10.0\tOFF\t0.1\tOFF\t0.1\n'
        'Excitation Filter, Laser, Power (%, Exp(ms), Laser2, Power2 (%), Laser3, Power3 (%) (1) :\tN/A\t560\t4\t10.0\tOFF\t0.1\tOFF\t0.1\n\n'
        'Cycle lasers :\tper Z\n\nZ motion :\t' + motion + '\n'
    )


IMAGES = [
    'Scan_Iter_0000_CamA_ch0_CAM1_stack0000_514nm_0000000msec_0136704211msecAbs_-01x_-01y_-01z_0000t.tif',
    'Scan_Iter_0000_CamB_ch1_CAM1_stack0000_560nm_0000000msec_0136704211msecAbs_-01x_-01y_-01z_0000t.tif',
]


class Workspace:
    """A temporary experiment folder (plus PSF folder) and a config on disk."""

    def __init__(self, test, objective_scanned=False):
        self.tmp = tempfile.TemporaryDirectory()
        test.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / 'Scan_Iter_0000_Settings.txt').write_text(mosaic_settings(objective_scanned))
        for name in IMAGES:
            (self.root / name).touch()
        psf = self.root / 'Calibration'
        psf.mkdir()
        for laser in ('514', '560'):
            for stem in ('cropped_%s_PSF', 'cropped_resampled_%s_PSF'):
                (psf / ((stem % laser) + '.tif')).touch()
                (psf / ((stem % laser) + '_Settings.txt')).write_text(mosaic_settings(True, interval=0.2))

    def config(self, **sections):
        cfg = {'paths': {'root': str(self.root)}, 'bdv': {'bdv_save': True},
               'bsub': {'o': '/dev/null', 'We': 10, 'n': 4, 'W': 480}}
        cfg['paths']['psf'] = {'dir': 'Calibration',
                               'laser': {'514': 'cropped_514_PSF.tif', '560': 'cropped_560_PSF.tif'},
                               'resampled': {'514': 'cropped_resampled_514_PSF.tif', '560': 'cropped_resampled_560_PSF.tif'}}
        cfg.update(sections)
        path = self.root / 'config.json'
        path.write_text(json.dumps(cfg))
        return path

    def commands(self, module, config_path):
        with contextlib.redirect_stdout(io.StringIO()):
            configs = module.load_configs(config_path)
        dirs = module.get_dirs(self.root, [str(self.root / 'Calibration')])
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            module.process(dirs, configs, dryrun=True, verbose=True)
        # one job line per image (decon-first also prints small psf-skew helper jobs)
        return [line for line in out.getvalue().splitlines() if line.startswith('bsub') and '-J psf-skew' not in line]


DESKEW = {'xy-res': XY, 'fill': 0.0, 'bit-depth': 16, 'angle': DESKEW_ANGLE}
DECON = {'xy-res': XY, 'n': 5, 'bit-depth': 16, 'subtract': 100.0}
MIP = {'x': True, 'y': True, 'z': True}


def rotate_command(line):
    """The `rotate ...` step of a job line, as a dict of flag -> value."""
    m = re.search(r'(?:^|&& |\')rotate ([^&]*)', line)
    assert m, line
    tokens = m.group(1).rstrip("' ").split()  # the whole job is quoted, so the last step ends with a quote
    flags, i = {}, 0
    while i < len(tokens):
        if tokens[i].startswith('-') and not re.fullmatch(r'-\d+(\.\d+)?', tokens[i]):
            if i + 1 < len(tokens) and (not tokens[i + 1].startswith('-') or re.fullmatch(r'-\d+(\.\d+)?(e-?\d+)?', tokens[i + 1])):
                flags[tokens[i]] = tokens[i + 1]
                i += 2
                continue
            flags[tokens[i]] = True
        i += 1
    flags['input'] = tokens[-1]
    return flags


def rotate_commands(line):
    """Every `rotate ...` step of a job line (one per rotated stage)."""
    steps = re.findall(r"(?:^|&& |')(rotate [^&]*)", line)
    return [rotate_command("'" + step.rstrip("' ") + "'") for step in steps]


class ConfigTests(unittest.TestCase):
    def load(self, module, **sections):
        ws = Workspace(self)
        path = ws.config(**sections)
        with contextlib.redirect_stdout(io.StringIO()) as out:
            configs = module.load_configs(path)
        return configs, out.getvalue()

    def test_defaults_follow_the_instrument(self):
        mosaic, _ = self.load(MOSAIC, deskew=dict(DESKEW), rotate={})
        self.assertEqual(mosaic['rotate']['angle'], {'flag': '-a', 'arg': -32.45})
        self.assertEqual(mosaic['rotate']['crop-mode'], {'flag': '-c', 'arg': 'keep-all'})
        llsm, _ = self.load(LLSM, deskew={'xy-res': 0.104}, rotate={})
        self.assertEqual(llsm['rotate']['angle'], {'flag': '-a', 'arg': 31.8})

    def test_options_become_flags_and_unsupported_keys_are_dropped(self):
        for module in (MOSAIC, LLSM):
            configs, out = self.load(module, deskew=dict(DESKEW), rotate={
                'angle': -32.45, 'crop-mode': 'strict', 'output-spacing': 0.2, 'fill': 1.0, 'bit-depth': 32, 'bogus': 3})
            rot = configs['rotate']
            self.assertIn('bogus', out)
            self.assertNotIn('bogus', rot)
            self.assertEqual(rot['crop-mode'], {'flag': '-c', 'arg': 'strict'})
            self.assertEqual(rot['output-spacing']['arg'], '0.2,0.2,0.2')
            self.assertEqual(rot['fill'], {'flag': '-f', 'arg': 1.0})
            self.assertEqual(rot['bit-depth'], {'flag': '-b', 'arg': 32})
            self.assertEqual(rot['executable_path'], 'rotate')

    def test_output_spacing_becomes_a_flag(self):
        for module in (MOSAIC, LLSM):
            for value, arg in (('input', 'input'), (0.2, '0.2,0.2,0.2'), (1, '1.0,1.0,1.0'), ([0.2, 0.2, 0.5], '0.2,0.2,0.5')):
                with self.subTest(module=module.__name__, value=value):
                    configs, _ = self.load(module, deskew=dict(DESKEW), rotate={'output-spacing': value})
                    self.assertEqual(configs['rotate']['output-spacing']['flag'], '--output-spacing')
                    self.assertEqual(configs['rotate']['output-spacing']['arg'], arg)
            configs, _ = self.load(module, deskew=dict(DESKEW), rotate={})
            self.assertNotIn('output-spacing', configs['rotate'])

    def test_isotropic_option_was_removed(self):
        for module in (MOSAIC, LLSM):
            configs, out = self.load(module, deskew=dict(DESKEW), rotate={'isotropic': False})
            self.assertIn('isotropic', out)  # reported as an unsupported option
            self.assertNotIn('isotropic', configs['rotate'])

    def test_anisotropic_xy_output_is_refused_with_mip(self):
        for module in (MOSAIC, LLSM):
            with self.subTest(module=module.__name__):
                with self.assertRaises(SystemExit) as cm:
                    self.load(module, deskew=dict(DESKEW), mip=dict(MIP), rotate={'output-spacing': [0.2, 0.3, 0.5]})
                self.assertIn('mip', str(cm.exception.code))
                # equal x and y is fine with mip, and any spacing is fine without mip
                self.load(module, deskew=dict(DESKEW), mip=dict(MIP), rotate={'output-spacing': [0.2, 0.2, 0.5]})
                self.load(module, deskew=dict(DESKEW), rotate={'output-spacing': [0.2, 0.3, 0.5]})

    def test_z_trim_becomes_a_flag(self):
        for module in (MOSAIC, LLSM):
            for value, arg in ((3, '3'), ([2, 5], '2,5'), ([0, 4], '0,4')):
                with self.subTest(module=module.__name__, value=value):
                    configs, _ = self.load(module, deskew=dict(DESKEW), rotate={'z-trim': value})
                    self.assertEqual(configs['rotate']['z-trim'], {'flag': '--z-trim', 'arg': arg})
            for value in (0, [0, 0]):  # nothing to trim: no flag
                configs, _ = self.load(module, deskew=dict(DESKEW), rotate={'z-trim': value})
                self.assertNotIn('z-trim', configs['rotate'])
            configs, _ = self.load(module, deskew=dict(DESKEW), rotate={})
            self.assertNotIn('z-trim', configs['rotate'])

    def test_invalid_values_exit(self):
        bad = [{'angle': 30}, {'angle': 400.0}, {'crop-mode': 'tight'}, {'output-spacing': 'iso'}, {'output-spacing': 0}, {'output-spacing': -1.0}, {'output-spacing': True},
               {'output-spacing': [0.2, 0.2]}, {'output-spacing': [0.2, 0.2, 0.0]}, {'output-spacing': [0.2, 'x', 0.2]},
               {'fill': 1}, {'bit-depth': 12}, {'xy-res': 1},
               {'z-trim': -1}, {'z-trim': 1.5}, {'z-trim': [1]}, {'z-trim': [1, -2]}, {'z-trim': [1, 2, 3]}, {'z-trim': True}]
        for module in (MOSAIC, LLSM):
            for rotate in bad:
                with self.subTest(module=module.__name__, rotate=rotate):
                    with self.assertRaises(SystemExit) as cm:
                        self.load(module, deskew=dict(DESKEW), rotate=rotate)
                    self.assertIn('ERROR', str(cm.exception.code))

    def test_strict_needs_deskew_geometry(self):
        for module in (MOSAIC, LLSM):
            with self.assertRaises(SystemExit) as cm:
                self.load(module, rotate={'crop-mode': 'strict'})
            self.assertIn('strict', str(cm.exception.code))
            self.load(module, deskew=dict(DESKEW), rotate={'crop-mode': 'strict'})  # fine with deskew

    def test_default_stages_rotate_the_last_stage_of_each_chain(self):
        cases = [
            (dict(deskew=dict(DESKEW)), ['deskew']),
            (dict(deskew=dict(DESKEW), decon=dict(DECON)), ['decon']),
            (dict(decon=dict(DECON)), ['decon']),
            ({'decon-first': {'decon': dict(DECON), 'deskew': dict(DESKEW)}}, ['deskew_after_decon']),
            ({'deskew': dict(DESKEW), 'decon-first': {'decon': dict(DECON), 'deskew': dict(DESKEW)}}, ['deskew', 'deskew_after_decon']),
            ({}, []),  # nothing else enabled: the unprocessed images are rotated
        ]
        for module in (MOSAIC, LLSM):
            for sections, expected in cases:
                with self.subTest(module=module.__name__, sections=list(sections)):
                    configs, _ = self.load(module, rotate={}, **sections)
                    self.assertEqual(configs['rotate']['stages'], expected)

    def test_explicit_stages(self):
        configs, _ = self.load(MOSAIC, deskew=dict(DESKEW), decon=dict(DECON), rotate={'stages': ['deskew', 'decon']})
        self.assertEqual(configs['rotate']['stages'], ['deskew', 'decon'])
        configs, _ = self.load(LLSM, deskew=dict(DESKEW), decon=dict(DECON), rotate={'stages': ['decon']})
        self.assertEqual(configs['rotate']['stages'], ['decon'])

    def test_invalid_stages_exit(self):
        bad = [
            ({'stages': 'deskew'}, 'list'),
            ({'stages': []}, 'list'),
            ({'stages': [3]}, 'list'),
            ({'stages': ['crop']}, 'must be deskew'),
            ({'stages': ['deskew', 'deskew']}, 'repeat'),
            ({'stages': ['decon']}, 'not enabled'),               # decon is not in the config
            ({'stages': ['deskew_after_decon']}, 'not enabled'),  # neither is decon-first
        ]
        for module in (MOSAIC, LLSM):
            for rotate, message in bad:
                with self.subTest(module=module.__name__, rotate=rotate):
                    with self.assertRaises(SystemExit) as cm:
                        self.load(module, deskew=dict(DESKEW), rotate=rotate)
                    self.assertIn(message, str(cm.exception.code))

    def test_strict_needs_deskew_geometry_for_every_rotated_stage(self):
        for module in (MOSAIC, LLSM):
            with self.assertRaises(SystemExit):  # decon only: no deskew geometry
                self.load(module, decon=dict(DECON), rotate={'crop-mode': 'strict'})
            self.load(module, deskew=dict(DESKEW), decon=dict(DECON), rotate={'crop-mode': 'strict'})  # decon after deskew is fine


class MosaicChainTests(unittest.TestCase):
    def check_geometry(self, flags):
        self.assertEqual(flags['-a'], '-32.45')
        self.assertAlmostEqual(float(flags['-x']), XY)
        self.assertAlmostEqual(float(flags['-z']), EXPECTED_Z, places=9)
        self.assertAlmostEqual(float(flags['-d']), EXPECTED_SHIFT, places=9)
        self.assertLess(float(flags['-d']), 0.0)  # MOSAIC deskew angle 147.55 -> negative shift

    def test_deskew_then_rotate(self):  # chain (a)
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), rotate={'angle': -32.45}, mip=dict(MIP)))
        self.assertEqual(len(lines), 2)
        for line in lines:
            self.assertLess(line.index('deskew -'), line.index('rotate -'))
            flags = rotate_command(line)
            self.check_geometry(flags)
            self.assertTrue(flags['input'].endswith('deskew/' + Path(flags['input']).name))
            self.assertTrue(flags['input'].endswith('_deskew.tif'))
            self.assertIn('/rotate_deskew/', line)
            self.assertIn('mip/rotate_deskew/', line)  # MIPs of the rotated data
            self.assertEqual(len(rotate_commands(line)), 1)
            self.assertNotIn('-i ', line)

    def test_deskew_decon_then_rotate(self):  # chain (b)
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), decon=dict(DECON), rotate={'angle': -32.45}))
        self.assertEqual(len(lines), 2)
        for line in lines:
            self.assertLess(line.index('deskew -'), line.index('decon -'))
            self.assertLess(line.index('decon -'), line.index('rotate -'))
            flags = rotate_command(line)
            self.check_geometry(flags)
            self.assertTrue(flags['input'].endswith('_decon.tif'))  # rotates the deconvolved data
            self.assertIn('/rotate_decon/', line)
            self.assertNotIn('rotate_deskew', line)  # by default only the last stage is rotated
            self.assertEqual(len(rotate_commands(line)), 1)

    def test_rotating_both_deskew_and_decon(self):
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), decon=dict(DECON), mip=dict(MIP),
                                              rotate={'angle': -32.45, 'stages': ['deskew', 'decon']}))
        self.assertEqual(len(lines), 2)
        for line in lines:
            first, second = rotate_commands(line)
            self.assertTrue(first['input'].endswith('_deskew.tif'))
            self.assertTrue(second['input'].endswith('_decon.tif'))
            self.assertIn('/rotate_deskew/', line)
            self.assertIn('/rotate_decon/', line)
            self.assertIn('mip/rotate_deskew/', line)
            self.assertIn('mip/rotate_decon/', line)
            for flags in (first, second):
                self.check_geometry(flags)  # same grid for both stages

    def test_decon_first_then_rotate(self):  # chain (c)
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(rotate={'angle': -32.45, 'crop-mode': 'strict'},
                                              **{'decon-first': {'decon': dict(DECON), 'deskew': dict(DESKEW)}}))
        self.assertEqual(len(lines), 2)
        for line in lines:
            self.assertLess(line.index('decon -'), line.index('deskew -'))
            self.assertLess(line.index('deskew -'), line.index('rotate -'))
            flags = rotate_command(line)
            self.check_geometry(flags)
            self.assertEqual(flags['-c'], 'strict')
            self.assertTrue(flags['input'].endswith('_deskew_after_decon.tif'))
            self.assertIn('/rotate_deskew_after_decon/', line)
            self.assertNotIn('/rotate/', line)

    def test_deskew_and_decon_first_chains_are_rotated_separately(self):
        ws = Workspace(self)
        other = {**DESKEW, 'xy-res': 0.1}  # the two deskew sections may differ; each chain keeps its own geometry
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), rotate={'angle': -32.45},
                                              **{'decon-first': {'decon': {**DECON, 'xy-res': 0.1}, 'deskew': other}}))
        self.assertEqual(len(lines), 2)
        for line in lines:
            first, second = rotate_commands(line)
            self.assertTrue(first['input'].endswith('_deskew.tif'))
            self.assertIn('/rotate_deskew/', line)
            self.assertTrue(second['input'].endswith('_deskew_after_decon.tif'))
            self.assertIn('/rotate_deskew_after_decon/', line)
            self.assertAlmostEqual(float(first['-x']), 0.108)
            self.assertAlmostEqual(float(second['-x']), 0.1)
            self.assertAlmostEqual(float(second['-d']), STAGE_STEP * math.cos(DESKEW_ANGLE * math.pi / 180.0) / 0.1, places=9)

    def test_standalone_rotation_of_objective_scanned_data(self):
        ws = Workspace(self, objective_scanned=True)
        lines = ws.commands(MOSAIC, ws.config(rotate={'angle': 20.0}))
        self.assertEqual(len(lines), 2)
        for line in lines:
            flags = rotate_command(line)
            self.assertEqual(flags['-a'], '20.0')
            self.assertAlmostEqual(float(flags['-z']), 0.2)  # xz-stage interval, no deskew involved
            self.assertAlmostEqual(float(flags['-x']), 0.108)  # MOSAIC default xy size
            self.assertNotIn('-d', flags)
            self.assertTrue(Path(flags['input']).name.startswith('Scan_Iter_0000_Cam'))
            self.assertIn('/rotate/', line)

    def test_standalone_rotation_of_stage_scanned_data_is_refused(self):
        ws = Workspace(self)  # stage scanned, no deskew in the config
        path = ws.config(rotate={'angle': 20.0})
        with self.assertRaises(SystemExit) as cm:
            ws.commands(MOSAIC, path)
        self.assertIn('objective-scanned', str(cm.exception.code))

    def test_default_output_spacing(self):
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), mip=dict(MIP), rotate={'angle': -32.45}))
        for line in lines:
            flags = rotate_command(line)
            self.assertNotIn('--output-spacing', flags)
            self.assertEqual(flags['-c'], 'keep-all')
            mip = re.search(r'mip [^&]*mip/rotate_deskew/[^&]*', line).group(0)
            self.assertIn('-p 0.108', mip)
            self.assertIn('-q 0.108', mip)  # rotated MIPs carry the isotropic z size

    def test_input_output_spacing(self):
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), mip=dict(MIP), rotate={'angle': -32.45, 'output-spacing': 'input'}))
        for line in lines:
            flags = rotate_command(line)
            self.assertEqual(flags['--output-spacing'], 'input')
            mip = re.search(r'mip [^&]*mip/rotate_deskew/[^&]*', line).group(0)
            self.assertNotIn('-q 0.108', mip)  # rotated MIPs carry the input z size
            self.assertIn('-q %s' % EXPECTED_Z, mip)

    def test_user_output_spacing_reaches_rotate_and_mip(self):
        ws = Workspace(self)
        lines = ws.commands(MOSAIC, ws.config(deskew=dict(DESKEW), mip=dict(MIP),
                                              rotate={'angle': -32.45, 'output-spacing': [0.2, 0.2, 0.4]}))
        for line in lines:
            self.assertEqual(rotate_command(line)['--output-spacing'], '0.2,0.2,0.4')
            mip = re.search(r'mip [^&]*mip/rotate_deskew/[^&]*', line).group(0)
            self.assertIn('-p 0.2 ', mip)
            self.assertIn('-q 0.4 ', mip)

    def test_spacing_warnings_are_printed_once(self):
        ws = Workspace(self)
        for module in (MOSAIC,):
            module.warned_rotate_spacing.clear()
            path = ws.config(deskew=dict(DESKEW), rotate={'angle': -32.45, 'output-spacing': 0.05})
            with contextlib.redirect_stdout(io.StringIO()) as out:
                configs = module.load_configs(path)
                dirs = module.get_dirs(ws.root, [str(ws.root / 'Calibration')])
                module.process(dirs, configs, dryrun=True)
            text = out.getvalue()
            self.assertEqual(text.count('rotate output-spacing x = 0.05 um is finer'), 1)  # two channels, one warning
            self.assertEqual(text.count('rotate output-spacing z = 0.05 um is finer'), 1)
            # 0.15 is finer than the input z spacing but not than xy: no warning
            module.warned_rotate_spacing.clear()
            path = ws.config(deskew=dict(DESKEW), rotate={'angle': -32.45, 'output-spacing': 0.15})
            with contextlib.redirect_stdout(io.StringIO()) as out:
                configs = module.load_configs(path)
                module.process(module.get_dirs(ws.root, [str(ws.root / 'Calibration')]), configs, dryrun=True)
            self.assertNotIn('finer', out.getvalue())

    def test_rotate_directory_and_processed_record(self):
        ws = Workspace(self)
        configs_path = ws.config(deskew=dict(DESKEW), rotate={'angle': -32.45})
        with contextlib.redirect_stdout(io.StringIO()):
            configs = MOSAIC.load_configs(configs_path)
            dirs = MOSAIC.get_dirs(ws.root, [str(ws.root / 'Calibration')])
            processed = MOSAIC.process(dirs, configs, dryrun=True)
        record = processed[str(ws.root)]
        self.assertIn('rotate', record)
        self.assertEqual(record['rotate']['angle']['arg'], -32.45)
        self.assertFalse((ws.root / 'rotate').exists())  # dry runs create nothing


if __name__ == '__main__':
    unittest.main()
