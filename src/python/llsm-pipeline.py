#! /usr/bin/python3

"""
llsm-pipeline
This program is the entry point for the LLSM pipeline.
"""

import argparse
import os
import re
import json
import math
import time
import datetime
from pathlib import Path, PurePath
from sys import exit
import settings2json

def parse_args():
    parser = argparse.ArgumentParser(description='Batch deskewing, deconvolution, and mip script for LLSM images.')
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
        supported_opts = ['o', 'We', 'n', 'P']
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

        if 'P' in configs['bsub']:
            configs['bsub']['P'] = {'flag': '-P', 'arg': configs['bsub']['P']}

# sanitize crop configs
    if 'crop' in configs:
        supported_opts = ['xy-res', 'crop', 'bit-depth', 'executable_path','cropTop','cropBottom','cropLeft','cropRight']
        for key in list(configs['crop']):
            if key not in supported_opts:
                print('warning: crop option \'%s\' in config.json is not supported' % key)
                del configs['crop'][key]

        if not 'executable_path' in configs['crop']:
            configs['crop']['executable_path'] = "crop"

        if 'xy-res' in configs['crop']:
            if not type(configs['crop']['xy-res']) is float:
                exit('error: crop xy resolution \'%s\' in config.json is not a float' % configs['crop']['xy-res'])
            configs['crop']['xy-res'] = {'flag': '-x', 'arg': configs['crop']['xy-res']}
        
        cropSize = [0,0,0,0] # If missing a side, assume zero cropping
        if "cropTop" in configs['crop']:
            cropSize[0] = configs['crop']['cropTop']
            del configs['crop']['cropTop']
        if "cropBottom" in configs['crop']:
            cropSize[1] = configs['crop']['cropBottom']
            del configs['crop']['cropBottom']
        if "cropLeft" in configs['crop']:
            cropSize[2] = configs['crop']['cropLeft']
            del configs['crop']['cropLeft']
        if "cropRight" in configs['crop']:
            cropSize[3] = configs['crop']['cropRight']
            del configs['crop']['cropRight']
        cropSize = [str(int) for int in cropSize]
        cropSize = ",".join(cropSize)

        configs['crop']['crop'] = {'flag': '-c', 'arg': cropSize}

        if 'bit-depth' in configs['crop']:
            if configs['crop']['bit-depth'] not in [8, 16, 32]:
                exit('error: crop bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['crop']['bit-depth'])
            configs['crop']['bit-depth'] = {'flag': '-b', 'arg': configs['crop']['bit-depth']}

    # sanitize deskew configs
    if 'deskew' in configs:
        supported_opts = ['xy-res', 'fill', 'bit-depth', 'angle', 'executable_path']
        for key in list(configs['deskew']):
            if key not in supported_opts:
                print('warning: deskew option \'%s\' in config.json is not supported' % key)
                del configs['deskew'][key]

        if not 'executable_path' in configs['deskew']:
            configs['deskew']['executable_path'] = "deskew"

        if 'angle' in configs['deskew']:
            if not type(configs['deskew']['angle']) is float:
                exit('error: deskew angle \'%s\' in config.json is not a float' % configs['deskew']['angle'])
        else:
            configs['deskew']['angle'] = 31.8 # Angle is 31.8 in LLSM but -32.45 for MOSAIC
        configs['deskew']['angle'] = {'flag': '-a', 'arg': configs['deskew']['angle']}

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
        supported_opts = ['n', 'bit-depth', 'subtract', 'executable_path']
        for key in list(configs['decon']):
            if key not in supported_opts:
                print('warning: decon option \'%s\' in config.json is not supported' % key)
                del configs['decon'][key]

        if not 'executable_path' in configs['decon']:
            configs['decon']['executable_path'] = "decon"

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

    # sanitize mip configs
    if 'mip' in configs:
        supported_opts = ['x', 'y', 'z', 'executable_path']
        for key in list(configs['mip']):
            if key not in supported_opts:
                print('warning: mip option \'%s\' in config.json is not supported' % key)
                del configs['mip'][key]

        if not 'executable_path' in configs['mip']:
            configs['mip']['executable_path'] = "mip"

        if 'x' in configs['mip']:
            if not type(configs['mip']['x']) is bool:
                exit('error: mip x projection \'%s\' in config.json is not a true or false' % configs['mip']['x'])
            if configs['mip']['x']:
                configs['mip']['x'] = {'flag': '-x'}
            else:
                del configs['mip']['x']
        
        if 'y' in configs['mip']:
            if not type(configs['mip']['y']) is bool:
                exit('error: mip y projection \'%s\' in config.json is not a true or false' % configs['mip']['y'])
            if configs['mip']['y']:
                configs['mip']['y'] = {'flag': '-y'}
            else:
                del configs['mip']['y']

        if 'z' in configs['mip']:
            if not type(configs['mip']['z']) is bool:
                exit('error: mip z projection \'%s\' in config.json is not a true or false' % configs['mip']['z'])
            if configs['mip']['z']:
                configs['mip']['z'] = {'flag': '-z'}
            else:
                del configs['mip']['z']

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
        if 'arg' in params[key]:
            cmd += ' %s %s' % (params[key]['flag'], params[key]['arg'])
        elif 'flag' in params[key]:
            cmd += ' %s ' % (params[key]['flag'])

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
    params_crop = {}

    cmd_bsub = None
    cmd_crop = None
    cmd_deskew = None
    cmd_decon = None
    cmd_mip = None

    # update default params with user defined configs and build commands
    if 'bsub' in configs:
        params_bsub.update(configs['bsub'])
        cmd_bsub = params2cmd(params_bsub, 'bsub')
    if 'crop' in configs:
        params_crop.update(configs['crop'])
        cmd_crop = params2cmd(params_crop, configs['crop']['executable_path'])
    if 'deskew' in configs:
        params_deskew.update(configs['deskew'])
        cmd_deskew = params2cmd(params_deskew, configs['deskew']['executable_path'])
    if 'decon' in configs:
        params_decon.update(configs['decon'])
        cmd_decon = params2cmd(params_decon, configs['decon']['executable_path'])
    if 'mip' in configs:
        params_mip.update(configs['mip'])
        cmd_mip = params2cmd(params_mip, configs['mip']['executable_path'])

    # parse PSF settings files
    if 'decon' in configs:
        root = Path(configs['paths']['root'])

        psf_settings = {}
        for laser, psf_filename in configs['paths']['psf']['laser'].items():
            psf_settings[laser] = {}
            psf_filename, _ = os.path.splitext(psf_filename)
            p = root / configs['paths']['psf']['dir'] / (psf_filename + '_Settings.txt')
            if not p.is_file():
                exit('error: laser %s psf file \'%s\' does not have a Settings file' % (laser, p))
   
            j = settings2json.parse_txt(p)

            if j['waveform']['z-motion'] == 'Sample piezo':
                exit('error: PSF is skewed and cannot be used for decon')
            elif j['waveform']['z-motion'] == 'Z galvo & piezo':
                psf_settings[laser]['z-step'] = j['waveform']['z-pzt']['interval'][0] 
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
                
        # crop setup
        crop = False
        if params_crop:
            if 's-piezo' in settings['waveform']:
                stepCrop = settings['waveform']['s-piezo']['interval']
            elif 'z-pzt' in settings['waveform']:
                stepCrop = settings['waveform']['z-pzt']['interval']
            else:
                exit('error: settings file did not contain an valid step parameter for cropping')

            crop = True

            # crop output directory
            output_crop = d / 'crop'
            if not dryrun:
                output_crop.mkdir(exist_ok=True)

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

        # mip setup
        mip = False
        if params_mip:

            mip = True

            # mip output directory
            output_mip = d / 'mip'
            if deskew:
                output_deskew_mip = output_mip / 'deskew'
                if not dryrun:
                    output_deskew_mip.mkdir(parents=True, exist_ok=True)
            if decon:
                output_decon_mip = output_mip / 'decon'
                if not dryrun:
                    output_decon_mip.mkdir(parents=True, exist_ok=True)

        # process all files in directory
        for f in files:
            m = pattern.fullmatch(f)
            if m:
                ch = int(m.group(1))
                cmd = [cmd_bsub, ' \"']

                if crop:
                    inpath = d / f
                    outpath = output_crop / tag_filename(f, '_crop')
                    tmp = cmd_crop + ' -w -s %s -o %s  %s;' % (stepCrop[ch], outpath, inpath) # note, input is steps b/c angle calculation repeated in deskew...
                    cmd.append(tmp)

                if deskew:
                    if crop:
                        inpath = output_crop / tag_filename(f, '_crop')
                    else:
                        inpath = d / f
                    outpath = output_deskew / tag_filename(f, '_deskew')
                    step = settings['waveform']['s-piezo']['interval'][ch] 
                    step = step * math.sin(31.8 * math.pi/180.0)

                    tmp = cmd_deskew + ' -w -s %s -o %s %s;' % (steps[ch], outpath, inpath)
                    cmd.append(tmp)

                    # create mips for deskew
                    if mip:
                        inpath = output_deskew / tag_filename(f, '_deskew')
                        outpath = output_deskew_mip / tag_filename(f, '_deskew_mip')
                        tmp = cmd_mip + ' -q %s -o %s %s;' % (step, outpath, inpath)
                        cmd.append(tmp)

                if decon:
                    if deskew:
                        inpath = output_deskew / tag_filename(f, '_deskew')
                    elif crop:
                        inpath = output_crop / tag_filename(f, '_crop')
                    else:
                        inpath = d / f

                    outpath = output_decon / tag_filename(f, '_decon')

                    if deskew:
                        step = settings['waveform']['s-piezo']['interval'][ch] 
                        step = step * math.sin(31.8 * math.pi/180.0) 
                    else:
                        step = settings['waveform']['z-pzt']['interval'][ch]

                    tmp = cmd_decon + ' -w -k %s -p %s -q %s -o %s %s;' % (psfpaths[ch], psfsteps[ch], step, outpath, inpath)
                    cmd.append(tmp)

                    # create mips for decon
                    if mip:
                        inpath = output_decon / tag_filename(f, '_decon')
                        outpath = output_decon_mip / tag_filename(f, '_decon_mip')
                        tmp = cmd_mip + ' -q %s -o %s %s;' % (step, outpath , inpath)
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
        if crop:
            processed[d]['crop'] = params_crop
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
