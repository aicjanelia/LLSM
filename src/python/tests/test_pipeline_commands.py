import sys
from pathlib import Path
import subprocess
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pipeline_commands import (build_job_command, parse_output_spacing,
                               rotate_output_spacing, rotate_spacing_warnings)


class PipelineCommandTests(unittest.TestCase):
    def run_job(self, steps):
        # sh -c stands in for the scheduler accepting a single job string.
        return subprocess.run(build_job_command('sh -c', steps), shell=True,
                              capture_output=True, text=True)

    def test_failed_decon_preserves_exit_and_skips_mip(self):
        result = self.run_job(["printf 'deskew done\\n';", "sh -c 'exit 7';",
                               "printf 'mip must not run\\n';"])
        self.assertEqual(result.returncode, 7)
        self.assertEqual(result.stdout, 'deskew done\n')

    def test_success_runs_all_steps_in_order(self):
        result = self.run_job(["printf 'deskew\\n';", "printf 'decon\\n';",
                               "printf 'mip\\n';"])
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, 'deskew\ndecon\nmip\n')

    def test_last_step_failure_is_not_masked(self):
        self.assertEqual(self.run_job(['true;', "sh -c 'exit 9';"]).returncode, 9)


class OutputSpacingTests(unittest.TestCase):
    def test_parse_forms(self):
        self.assertEqual(parse_output_spacing('input')['arg'], 'input')
        self.assertEqual(parse_output_spacing(0.2)['values'], [0.2, 0.2, 0.2])
        self.assertEqual(parse_output_spacing(1)['values'], [1.0, 1.0, 1.0])  # JSON ints are accepted
        self.assertEqual(parse_output_spacing([0.2, 0.2, 0.5])['arg'], '0.2,0.2,0.5')
        self.assertEqual(parse_output_spacing(0.2)['flag'], '--output-spacing')

    def test_parse_rejects_bad_values(self):
        for bad in (0, -0.2, True, 'iso', [0.2, 0.2], [0.2, 0.2, 0.2, 0.2], [0.2, 0.0, 0.2],
                    [0.2, 'a', 0.2], None, float('nan'), float('inf')):
            with self.assertRaises(ValueError, msg=repr(bad)):
                parse_output_spacing(bad)

    def test_resolved_spacing(self):
        self.assertEqual(rotate_output_spacing({}, 0.1, 0.3), (0.1, 0.1, 0.1))
        self.assertEqual(rotate_output_spacing({'output-spacing': parse_output_spacing('input')}, 0.1, 0.3), (0.1, 0.1, 0.3))
        self.assertEqual(rotate_output_spacing({'output-spacing': parse_output_spacing(0.2)}, 0.1, 0.3), (0.2, 0.2, 0.2))
        self.assertEqual(rotate_output_spacing({'output-spacing': parse_output_spacing([0.2, 0.2, 0.5])}, 0.1, 0.3), (0.2, 0.2, 0.5))

    def test_warnings(self):
        def warn(value, xy=0.1, z=0.3):
            return rotate_spacing_warnings({'output-spacing': parse_output_spacing(value)}, xy, z)
        self.assertEqual(rotate_spacing_warnings({}, 0.1, 0.3), [])      # default never warns
        self.assertEqual(warn('input'), [])
        self.assertEqual(warn(0.1), [])                                   # equals the smallest input spacing (xy)
        self.assertEqual(warn(0.15), [])                                  # finer than input z but not than xy: no warning
        self.assertEqual(warn([0.1, 0.1, 0.05]), [warn(0.05)[2]])         # only z is below the smallest input spacing
        self.assertEqual(warn([0.2, 0.2, 0.3]), [])
        self.assertEqual(warn(0.3), [])
        finer = warn(0.05)                                                # finer than xy on all three axes
        self.assertEqual(len(finer), 3)
        self.assertTrue(all('finer' in w for w in finer))
        coarser = warn([0.1, 0.1, 0.5])                                   # z coarser than the input z spacing
        self.assertEqual(len(coarser), 1)
        self.assertIn('coarser', coarser[0])
        self.assertEqual(sum('coarser' in w for w in warn(0.5)), 3)       # x, y and z all exceed the input z
        # input z finer than input xy: the smallest input spacing is then z
        self.assertEqual(rotate_spacing_warnings({'output-spacing': parse_output_spacing(0.1)}, 0.2, 0.1), [])
        self.assertEqual(len(rotate_spacing_warnings({'output-spacing': parse_output_spacing(0.05)}, 0.2, 0.1)), 3)


if __name__ == '__main__':
    unittest.main()
