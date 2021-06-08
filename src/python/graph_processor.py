import os
import re
import math
import time
import datetime
from pathlib import Path, PurePath

import settings2json


def params2cmd(params, cmd_name):
    cmd = cmd_name
    for key in params:
        if 'arg' in params[key]:
            cmd += f' %s %s' % (params[key]['flag'], params[key]['arg'])
        else:
            cmd += f' %s ' % (params[key]['flag'])

    return cmd

def process(dirs, configs, dryrun=False, verbose=False):
    processed = {}
    params_bsub = {
        'J': {
            'flag': '-J',
            'arg': 'llsm-pipeline'
        },
        'o': {
            'flag': '-o',
            'arg': '/dev/null'
        },
        'We': {
            'flag': '-We',
            'arg': 10
        },
        'n': {
            'flag': '-n',
            'arg': 4
        }
    }
    params_deskew = {}
    params_decon = {}
    params_mip = {}

    # update default params with user defined configs
    if 'bsub' in configs:
        params_bsub.update(configs['bsub'])
    if 'deskew' in configs:
        params_deskew.update(configs['deskew'])
    if 'decon' in configs:
        params_decon.update(configs['decon'])
    if 'mip' in configs:
        params_mip.update(configs['mip'])

    # build commands
    cmd_bsub = params2cmd(params_bsub, 'bsub')
    cmd_deskew = params2cmd(params_deskew, 'deskew')
    cmd_decon = params2cmd(params_decon, 'decon')
    cmd_mip = params2cmd(params_mip, 'mip')

    # parse PSF settings files
    if 'decon' in configs:
        root = Path(configs["paths"]["root"])

        psf_settings = {}
        for key, val in configs['paths']['psf']['laser'].items():
            psf_settings[key] = {}
            filename, _ = os.path.splitext(val)
            p = root / configs['paths']['psf']['dir'] / (filename + '_Settings.txt')
            if not p.is_file():
                exit(f'error: laser %s psf file \'%s\' does not have a Settings file' % (key, p))
   
            settings = settings2json.parse_txt(p)

            if settings['waveform']['z-motion'] == 'Sample piezo':
                step = settings['waveform']['s-piezo']['interval'][0] 
                psf_settings[key]['z-step'] = step * math.sin(31.8 * math.pi/180.0)
            elif settings['waveform']['z-motion'] == 'Z galvo & piezo':
                psf_settings[key]['z-step'] = settings['waveform']['z-pzt']['interval'][0] 
            else:
                exit(f'error: PSF z-motion cannot be determined for laser %s' % key)

    # process each directory
    for d in dirs:
        print(f'processing \'%s\'...' % d)

        # get list of files
        files = os.listdir(d)

        # parse the last Settings.txt file
        for f in reversed(files):
            if f.endswith('Settings.txt'):
                print(f'parsing \'%s\'...' % f)
                settings = settings2json.parse_txt(d / f)
                pattern = re.compile(f.split('_')[0] + '.*_ch(\d+).*\.tif')
                break

        # check for z-motion settings
        if 'waveform' not in settings:
            exit(f'error: settings file did not contain a Waveform section')
        if 'z-motion' not in settings['waveform']:
            exit(f'error: settings file did not contain a Z motion field')

        # save settings json
        if not dryrun:
            with open(d / 'settings.json', 'w') as path:
                json.dump(settings, path, indent=4)

        # deskew setup
        deskew = False
        if settings['waveform']['z-motion'] == 'Sample piezo':
            if 's-piezo' not in settings['waveform']:
                exit(f'error: settings file did not contain a S Piezo field')
            if 'interval' not in settings['waveform']['s-piezo']:
                exit(f'error: settings file did not contain a S Piezo Interval field')

            deskew = True

            # get step sizes
            steps = settings['waveform']['s-piezo']['interval']

            # deskew output directory
            output_deskew = d / 'deskew'
            if not dryrun:
                output_deskew.mkdir(exist_ok=True)

        # decon setup
        decon = False
        if params_decon:
            if 'laser' not in settings['waveform']:
                exit(f'error: settings file did not contain a Laser field')

            decon = True

            root = Path(configs['paths']['root'])
            psfpaths = []
            psfsteps = []
            for laser in settings['waveform']['laser']:
                filename = configs['paths']['psf']['laser'][str(laser)]
                psfpaths.append(root / configs['paths']['psf']['dir'] / filename )
                psfsteps.append(psf_settings[str(laser)]['z-step'])

            # decon output directory
            output_decon = d / 'decon'
            if not dryrun:
                output_decon.mkdir(exist_ok=True)

        # mip setup
        mip = False
        if params_mip:

            mip = True

            # mip output directory
            output_mip = d / 'mip'
            if not dryrun:
                output_mip.mkdir(exist_ok=True)
            if deskew:
                output_deskew_mip = output_mip / 'deskew'
                if not dryrun:
                    output_deskew_mip.mkdir(exist_ok=True)
            if decon:
                output_decon_mip = output_mip / 'decon'
                if not dryrun:
                    output_decon_mip.mkdir(exist_ok=True)

        # process all files in directory
        for f in files:
            m = pattern.fullmatch(f)
            if m:
                ch = int(m.group(1))
                cmd = [cmd_bsub, ' \"']

                if deskew:
                    inpath = d / f
                    outpath = output_deskew / tag_filename(f, '_deskew')
                    tmp = cmd_deskew + f' -w -s %s -o %s %s;' % (steps[ch], outpath , inpath)
                    cmd.append(tmp)

                    # create mips for deskew
                    if mip:
                        inpath = output_deskew / tag_filename(f, '_deskew')
                        outpath = output_deskew_mip / tag_filename(f, '_deskew_mip')
                        tmp = cmd_mip + f' -o %s %s;' % (outpath , inpath)
                        cmd.append(tmp)

                if decon:
                    if deskew:
                        inpath = output_deskew / tag_filename(f, '_deskew')
                    else:
                        inpath = d / f

                    outpath = output_decon / tag_filename(f, '_decon')
                    step = settings['waveform']['s-piezo']['interval'][ch] 
                    step = step * math.sin(31.8 * math.pi/180.0)

                    tmp = cmd_decon + f' -w -k %s -p %s -q %s -o %s %s;' % (psfpaths[ch], psfsteps[ch], step, outpath, inpath)
                    cmd.append(tmp)

                    # create mips for decon
                    if mip:
                        inpath = output_decon / tag_filename(f, '_decon')
                        outpath = output_decon_mip / tag_filename(f, '_decon_mip')
                        tmp = cmd_mip + f' -o %s %s;' % (outpath , inpath)
                        cmd.append(tmp)

                if len(cmd) > 1:
                    cmd = ''.join(cmd)
                    cmd = cmd + '\"'
                    if verbose:
                        print(cmd)
                    if not dryrun:
                        os.system(cmd)
        
        # update processed list
        d = str(d)
        processed[d] = {}
        processed[d]['time'] = datetime.datetime.fromtimestamp(time.time()).isoformat()
        if deskew:
            processed[d]['deskew'] = params_deskew
            processed[d]['deskew']['step'] = steps
        if decon:
            processed[d]['decon'] = params_decon
        if decon:
            processed[d]['decon'] = params_decon
        if mip:
            processed[d]['mip'] = params_mip 

    return  processed