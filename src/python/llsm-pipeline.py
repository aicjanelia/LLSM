#! /misc/local/python-3.8.2/bin/python3
"""
llsm-pipeline
This program is the entry point for the LLSM pipeline.
"""

import argparse
import os
import re
import json
import math
from pathlib import Path, PurePath
from sys import exit
import settings2json

def parse_args():
    parser = argparse.ArgumentParser(description='Batch deskewing, deconvolution, andmip script for LLSM images.')
    parser.add_argument('input', type=Path, help='path to configuration JSON file')
    parser.add_argument('--dry-run', '-d', default=False, action='store_true', dest='dryrun',help='execute without submitting any bsub jobs')
    parser.add_argument('--verbose', '-v', default=False, action='store_true', dest='verbose',help='print details (including commands to bsub)')
    args = parser.parse_args()

    if not args.input.is_file():
        exit('error: \'%s\' does not exist' % args.input)

    if not args.input.suffix == '.json':
        print('warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

def load_configs(path):
    with path.open(mode='r') as f:
        try:
            configs = json.load(f)
        except json.JSONDecodeError as e:
            exit('error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))

    # sanitize root path
    root = Path(configs["paths"]["root"])
    if not root.is_dir():
        exit('error: root path \'%s\' does not exist' % root)

    # sanitize bsub configs
    if 'bsub' in configs:
        supported_opts = ['o', 'We', 'n']
        for key in list(configs['bsub']):
            if key not in supported_opts:
                print('warning: bsub option \'%s\' in config.json is not supported' % key)
                del configs['bsub'][key]

        if 'o' in configs['bsub']:
            if not Path(configs['bsub']['o']).is_dir:
                exit('error: bsub output directory \'%s\' in config.json is not a valid path' % configs['bsub']['o'])
            configs['bsub']['o'] = {'flag': '-o', 'arg': configs['bsub']['o']}
        
        if 'We' in configs['bsub']:
            if not type(configs['bsub']['We']) is int:
                exit('error: bsub estimated wait time \'%s\' in config.json is not an integer' % configs['bsub']['We'])
            configs['bsub']['We'] = {'flag': '-We', 'arg': configs['bsub']['We']}
        
        if 'n' in configs['bsub']:
            if not type(configs['bsub']['n']) is int:
                exit('error: bsub slot count \'%s\' in config.json is not an integer' % configs['bsub']['n'])
            configs['bsub']['n'] = {'flag': '-n', 'arg': configs['bsub']['n']}

    # sanitize deskew configs
    if 'deskew' in configs:
        supported_opts = ['xy-res', 'fill', 'bit-depth']
        for key in list(configs['deskew']):
            if key not in supported_opts:
                print('warning: deskew option \'%s\' in config.json is not supported' % key)
                del configs['deskew'][key]

        if 'xy-res' in configs['deskew']:
            if not type(configs['deskew']['xy-res']) is float:
                exit('error: deskew xy resolution \'%s\' in config.json is not a float' % configs['deskew']['xy-res'])
            configs['deskew']['xy-res'] = {'flag': '-x', 'arg': configs['deskew']['xy-res']}
        
        if 'fill' in configs['deskew']:
            if not type(configs['deskew']['fill']) is float:
                exit('error: deskew background fill value \'%s\' in config.json is not a float' % configs['deskew']['fill'])
            configs['deskew']['fill'] = {'flag': '-f', 'arg': configs['deskew']['fill']}

        if 'bit-depth' in configs['deskew']:
            if configs['deskew']['bit-depth'] not in [8, 16, 32]:
                exit('error: deskew bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['deskew']['bit-depth'])
            configs['deskew']['bit-depth'] = {'flag': '-b', 'arg': configs['deskew']['bit-depth']}

    # sanitize decon configs
    if 'decon' in configs:
        supported_opts = ['n', 'bit-depth', 'subtract']
        for key in list(configs['decon']):
            if key not in supported_opts:
                print('warning: decon option \'%s\' in config.json is not supported' % key)
                del configs['decon'][key]

        if 'n' in configs['decon']:
            if not type(configs['decon']['n']) is int:
                exit('error: decon iteration number (n) \'%s\' in config.json must be 8, 16, or 32' % configs['decon']['n'])
            configs['decon']['n'] = {'flag': '-n', 'arg': configs['decon']['n']}
        
        if 'bit-depth' in configs['decon']:
            if configs['decon']['bit-depth'] not in [8, 16, 32]:
                exit('error: decon bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['decon']['bit-depth'])
            configs['decon']['bit-depth'] = {'flag': '-b', 'arg': configs['decon']['bit-depth']}

        if 'subtract' in configs['decon']:
            if not type(configs['decon']['subtract']) is float:
                exit('error: decon subtract value \'%s\' in config.json must be a float' % configs['decon']['subtract'])
            configs['decon']['subtract'] = {'flag': '-s', 'arg': configs['decon']['subtract']}

        if 'psf' not in configs['paths']:
            exit('error: decon enabled, but no psf parameters found in config file')

        if 'dir' in configs['paths']['psf']:
            partial_path = root / configs['paths']['psf']['dir']
        else:
            configs['paths']['psf']['dir'] = None
            partial_path = root
            print('warning: no psf directory provided... using \'%s\'' % root)
        
        if 'laser' not in configs['paths']['psf']:
            exit('error: no psf files provided')

        for key, val in configs['paths']['psf']['laser'].items():
            p = partial_path / val
            if not p.is_file():
                exit('error: laser %s psf file \'%s\' does not exist' % (key, p))

    return configs

def get_processed_json(path):
    if path.is_file():
        with path.open(mode='r') as f:
            try:
                processed = json.load(f)
            except json.JSONDecodeError as e:
                exit('error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
        
        return processed
    else:
        return {}

def get_dirs(path, excludes):
    unprocessed_dirs = []

    for root, dirs, files in os.walk(path):
        root = Path(root)

        # update directories to prevent us from traversing processed data
        dirs[:] = [d for d in dirs if str(root / d) not in excludes]

        # check if root contains a Settings.txt file
        for f in files:
            if f.endswith('Settings.txt'):
                unprocessed_dirs.append(root)
                break

    return unprocessed_dirs


def tag_filename(filename, string):
    f = PurePath(filename)
    return f.stem + string + f.suffix

def params2cmd(params, cmd_name):
    cmd = cmd_name
    for key in params:
        cmd += ' %s %s' % (params[key]['flag'], params[key]['arg'])

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

    # update default params with user defined configs
    if 'bsub' in configs:
        params_bsub.update(configs['bsub'])
    if 'deskew' in configs:
        params_deskew.update(configs['deskew'])
    if 'decon' in configs:
        params_decon.update(configs['decon'])

    # build commands
    cmd_bsub = params2cmd(params_bsub, 'bsub')
    cmd_deskew = params2cmd(params_deskew, 'deskew')
    cmd_decon = params2cmd(params_decon, 'decon')

    # parse PSF settings files
    if 'decon' in configs:
        root = Path(configs["paths"]["root"])

        psf_settings = {}
        for key, val in configs['paths']['psf']['laser'].items():
            psf_settings[key] = {}
            filename, _ = os.path.splitext(val)
            p = root / configs['paths']['psf']['dir'] / (filename + '_Settings.txt')
            if not p.is_file():
                exit('error: laser %s psf file \'%s\' does not have a Settings file' % (key, p))
   
            settings = settings2json.parse_txt(p)

            if settings['waveform']['z-motion'] == 'Sample piezo':
                step = settings['waveform']['s-piezo']['interval'][0] 
                psf_settings[key]['z-step'] = step * math.sin(31.8 * math.pi/180.0)
            elif settings['waveform']['z-motion'] == 'Z galvo & piezo':
                psf_settings[key]['z-step'] = settings['waveform']['z-pzt']['interval'][0] 
            else:
                exit('error: PSF z-motion cannot be determined for laser %s' % key)

    # process each directory
    for d in dirs:
        print('processing \'%s\'...' % d)

        # get list of files
        files = os.listdir(d)

        # parse the last Settings.txt file
        for f in reversed(files):
            if f.endswith('Settings.txt'):
                print('parsing \'%s\'...' % f)
                settings = settings2json.parse_txt(d / f)
                pattern = re.compile(f.split('_')[0] + '.*_ch(\d+).*\.tif')
                break

        # check for z-motion settings
        if 'waveform' not in settings:
            exit('error: settings file did not contain a Waveform section')
        if 'z-motion' not in settings['waveform']:
            exit('error: settings file did not contain a Z motion field')

        # save settings json
        if not dryrun:
            with open(d / 'settings.json', 'w') as path:
                json.dump(settings, path, indent=4)

        # deskew setup
        deskew = False
        if settings['waveform']['z-motion'] == 'Sample piezo':
            if 's-piezo' not in settings['waveform']:
                exit('error: settings file did not contain a S Piezo field')
            if 'interval' not in settings['waveform']['s-piezo']:
                exit('error: settings file did not contain a S Piezo Interval field')

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
                exit('error: settings file did not contain a Laser field')

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

        # process all files in directory
        for f in files:
            m = pattern.fullmatch(f)
            if m:
                ch = int(m.group(1))
                cmd = [cmd_bsub, ' \"']

                if deskew:
                    inpath = d / f
                    outpath = output_deskew / tag_filename(f, '_deskew')
                    tmp = cmd_deskew + ' -w -s %s -o %s %s;' % (steps[ch], outpath , inpath)
                    cmd.append(tmp)
                if decon:
                    if deskew:
                        inpath = output_deskew / tag_filename(f, '_deskew')
                    else:
                        inpath = d / f

                    outpath = output_decon / tag_filename(f, '_decon')
                    step = settings['waveform']['s-piezo']['interval'][ch] 
                    step = step * math.sin(31.8 * math.pi/180.0)

                    tmp = cmd_decon + ' -w -k %s -p %s -q %s -o %s %s;' % (psfpaths[ch], psfsteps[ch], step, outpath, inpath)
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
        if deskew or decon:
            processed[d] = {}
        if deskew:
            processed[d]['deskew'] = params_deskew
            processed[d]['deskew']['step'] = steps
        if decon:
            processed[d]['decon'] = params_decon

    return  processed

if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # load config file
    configs = load_configs(args.input)

    # get dictionary of processed files
    root_dir = Path(configs['paths']['root'])
    processed_path = root_dir / 'processed.json'
    processed_json = get_processed_json(processed_path)

    # get all directories containing a Settings files
    excludes = list(processed_json)
    if configs['paths']['psf']['dir'] != None:
        excludes.append(str(root_dir / configs['paths']['psf']['dir']))

    unprocessed_dirs = get_dirs(root_dir, excludes)

    # process images in new directories
    processed_dirs = process(unprocessed_dirs, configs, dryrun=args.dryrun, verbose=args.verbose)

    # update processed.json
    processed_json.update(processed_dirs)
    if not args.dryrun:
        with processed_path.open(mode='w') as f:
            f.write(json.dumps(processed_json, indent=4))

    if args.verbose:
        print('processed.json...')
        print(json.dumps(processed_json, indent=4))

    print('Done')
