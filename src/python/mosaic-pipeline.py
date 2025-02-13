#! /usr/bin/python3
"""
mosaic-pipeline
This program is the entry point for the MOSAIC pipeline.
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
# import sys
import mosaicsettings2json
from time import sleep

def parse_args():
    parser = argparse.ArgumentParser(description='Batch processing script for MOSAIC images.')
    parser.add_argument('input', type=Path, help='path to configuration JSON file')
    parser.add_argument('--dry-run', '-d', default=False, action='store_true', dest='dryrun',help='execute without submitting any bsub jobs')
    parser.add_argument('--verbose', '-v', default=False, action='store_true', dest='verbose',help='print details (including commands to bsub)')
    args = parser.parse_args()

    if not args.input.is_file():
        exit('ERROR: \'%s\' does not exist' % args.input)

    if not args.input.suffix == '.json':
        print('WARNING: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

def load_configs(path):
    with path.open(mode='r') as f:
        try:
            configs = json.load(f)
        except json.JSONDecodeError as e:
            exit('ERROR: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))

    # sanitize root path
    root = Path(configs["paths"]["root"])
    if not root.is_dir():
        exit('ERROR: root path \'%s\' does not exist' % root)

    # sanitize bsub configs
    if 'bsub' in configs:
        supported_opts = ['o', 'We', 'n', 'P','W']
        for key in list(configs['bsub']):
            if key not in supported_opts:
                print('WARNING: bsub option \'%s\' in config.json is not supported' % key)
                del configs['bsub'][key]

        if 'o' in configs['bsub']:
            if not Path(configs['bsub']['o']).is_dir:
                exit('ERROR: bsub output directory \'%s\' in config.json is not a valid path' % configs['bsub']['o'])
            configs['bsub']['o'] = {'flag': '-o', 'arg': configs['bsub']['o']}
        
        if 'We' in configs['bsub']:
            if not type(configs['bsub']['We']) is int:
                exit('ERROR: bsub estimated wait time \'%s\' in config.json is not an integer' % configs['bsub']['We'])
            configs['bsub']['We'] = {'flag': '-We', 'arg': configs['bsub']['We']}
        
        if 'n' in configs['bsub']:
            if not type(configs['bsub']['n']) is int:
                exit('ERROR: bsub slot count \'%s\' in config.json is not an integer' % configs['bsub']['n'])
            configs['bsub']['n'] = {'flag': '-n', 'arg': configs['bsub']['n']}

        if 'P' in configs['bsub']:
            configs['bsub']['P'] = {'flag': '-P', 'arg': configs['bsub']['P']}

        if 'W' in configs['bsub']:
            if not type(configs['bsub']['W']) is int:
                exit('ERROR: bsub hard run time limit \'%s\' in config.json is not an integer' % configs['bsub']['We'])
            configs['bsub']['W'] = {'flag': '-W', 'arg': configs['bsub']['W']}
        else:
            print('WARNING: Automatic hard run time limit set to 8 h. Add a W value to bsub in your config file to allow files to process longer than 8 h.')
            configs['bsub']['W'] = {'flag': '-W', 'arg': 8*60}

    # sanitize flatfield configs
    if 'flatfield' in configs:
        supported_opts = ['xy-res', 'bit-depth', 'executable_path']
        for key in list(configs['flatfield']):
            if key not in supported_opts:
                print('WARNING: flatfield option \'%s\' in config.json is not supported' % key)
                del configs['flatfield'][key]

        if not 'executable_path' in configs['flatfield']:
            configs['flatfield']['executable_path'] = "flatfield"

        if 'xy-res' in configs['flatfield']:            
            if not type(configs['flatfield']['xy-res']) is float:
                exit('ERROR: flatfield xy resolution \'%s\' in config.json is not a float' % configs['flatfield']['xy-res'])
            configs['xy-res'] = configs['flatfield']['xy-res']
            del configs['flatfield']['xy-res']
        else:
            configs['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        # Don't actually want to flatfield to use the xy res because that leads to deskew problems.
        # Keep the variable so that MIPs can complete correctly, but don't add it as an argument.

        if 'bit-depth' in configs['flatfield']:
            if configs['flatfield']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: flatfield bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['flatfield']['bit-depth'])
            configs['flatfield']['bit-depth'] = {'flag': '-b', 'arg': configs['flatfield']['bit-depth']}

        # check files
        if 'flatfield' not in configs['paths']:
            exit('ERROR: flatfield enabled, but no flatfield file parameters found in paths')

        if 'dir' in configs['paths']['flatfield']:
            flat_path = root / configs['paths']['flatfield']['dir']
        else:
            configs['paths']['flatfield']['dir'] = None
            flat_path = root
            print('WARNING: no flatfield directory provided... using \'%s\'' % root)

        if 'dark' not in configs['paths']['flatfield']:
            exit('ERROR: not dark image provided for flatfield correction')
        p = flat_path / configs['paths']['flatfield']['dark']
        if not p.is_file():
                exit('ERROR: dark image file \'%s\' does not exist' % (p))
        
        if 'laser' not in configs['paths']['flatfield']:
            exit('ERROR: no normalized flatfield images found in config')

        for key, val in configs['paths']['flatfield']['laser'].items():
            p = flat_path / val
            if not p.is_file():
                exit('ERROR: laser %s normalized flatfield file \'%s\' does not exist' % (key, p))

    # sanitize crop configs
    if 'crop' in configs:
        supported_opts = ['xy-res', 'crop', 'bit-depth', 'executable_path','cropTop','cropBottom','cropLeft','cropRight','cropFront','cropBack']
        for key in list(configs['crop']):
            if key not in supported_opts:
                print('WARNING: crop option \'%s\' in config.json is not supported' % key)
                del configs['crop'][key]

        if not 'executable_path' in configs['crop']:
            configs['crop']['executable_path'] = "crop"

        if 'xy-res' in configs['crop']:            
            if not type(configs['crop']['xy-res']) is float:
                exit('ERROR: crop xy resolution \'%s\' in config.json is not a float' % configs['crop']['xy-res'])

            configs['xy-res'] = configs['crop']['xy-res']
            del configs['crop']['xy-res']
        else:
            configs['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        # configs['crop']['xy-res'] = {'flag': '-x', 'arg': configs['crop']['xy-res']}
        # Don't actually want to crop to use the xy res because that leads to deskew problems.
        # Keep the variable so that MIPs can complete correctly, but don't add it as an argument.
        
        cropSize = [0,0,0,0,0,0] # If missing a side, assume zero cropping
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
        if "cropFront" in configs['crop']:
            cropSize[4] = configs['crop']['cropFront']
            del configs['crop']['cropFront']
        if "cropBack" in configs['crop']:
            cropSize[5] = configs['crop']['cropBack']
            del configs['crop']['cropBack']
        cropSize = [str(int) for int in cropSize]
        cropSize = ",".join(cropSize)

        configs['crop']['crop'] = {'flag': '-c', 'arg': cropSize}

        if 'bit-depth' in configs['crop']:
            if configs['crop']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: crop bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['crop']['bit-depth'])
            configs['crop']['bit-depth'] = {'flag': '-b', 'arg': configs['crop']['bit-depth']}

    # sanitize decon-first configs
    if 'decon-first' in configs:
        if not 'decon' in configs['decon-first']:
            exit('ERROR: provide deconvolution parameters for deconvolution followed by deskewing (''decon-first'')')
        if not 'deskew' in configs['decon-first']:
            exit('ERROR: provide deskewing parameters for deconovolution followed by desekewing (''decon-first'')')

        # deskew configs for decon-first
        supported_opts = ['xy-res', 'fill', 'bit-depth', 'angle', 'executable_path']
        for key in list(configs['decon-first']['deskew']):
            if key not in supported_opts:
                print('WARNING: decon-first deskew option \'%s\' in config.json is not supported' % key)
                del configs['decon-first']['deskew'][key]

        if not 'executable_path' in configs['decon-first']['deskew']:
            configs['decon-first']['deskew']['executable_path'] = "deskew"

        if 'angle' in configs['decon-first']['deskew']:
            if not type(configs['decon-first']['deskew']['angle']) is float:
                exit('ERROR: decon-first deskew angle \'%s\' in config.json is not a float' % configs['decon-first']['deskew']['angle'])
        else:
            configs['decon-first']['deskew']['angle'] = 147.55 # Angle is 31.8 in LLSM but -32.45 for MOSAIC
            print('WARNING: Using default MOSAIC angle of 147.55 for deskewing after deconvolution')
        configs['decon-first']['deskew']['angle'] = {'flag': '-a', 'arg': configs['decon-first']['deskew']['angle']}

        if 'xy-res' in configs['decon-first']['deskew']:
            if not type(configs['decon-first']['deskew']['xy-res']) is float:
                exit('ERROR: decon-first deskew xy resolution \'%s\' in config.json is not a float' % configs['decon-first']['deskew']['xy-res'])
        else:
            print('WARNING: Using default MOSAIC pixel size of 0.108 um/pixel for deskewing after deconvolution')
            configs['decon-first']['deskew']['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        configs['decon-first']['deskew']['xy-res'] = {'flag': '-x', 'arg': configs['decon-first']['deskew']['xy-res']}
        
        if 'fill' in configs['decon-first']['deskew']:
            if not type(configs['decon-first']['deskew']['fill']) is float:
                exit('ERROR: decon-first deskew background fill value \'%s\' in config.json is not a float' % configs['decon-first']['deskew']['fill'])            
        else:
            print('WARNING: Assuming a default deskew fill of 0.0 for decon-first deskewing steps.')
            configs['decon-first']['deskew']['fill'] = 0.0
        configs['decon-first']['deskew']['fill'] = {'flag': '-f', 'arg': configs['decon-first']['deskew']['fill']}

        if 'bit-depth' in configs['decon-first']['deskew']:
            if configs['decon-first']['deskew']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: decon-first deskew bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['decon-first']['deskew']['bit-depth'])
        else:
            print('WARNING: Assuming a default bit depth of 16 for decon-first deskewing.')
            configs['decon-first']['deskew']['bit-depth'] = 16
        configs['decon-first']['deskew']['bit-depth'] = {'flag': '-b', 'arg': configs['decon-first']['deskew']['bit-depth']}

        # decon configs for decon-first
        supported_opts = ['xy-res','n', 'bit-depth', 'subtract', 'executable_path']
        for key in list(configs['decon-first']['decon']):
            if key not in supported_opts:
                print('WARNING: decon-first decon option \'%s\' in config.json is not supported' % key)
                del configs['decon-first']['decon'][key]

        if not 'executable_path' in configs['decon-first']['decon']:
            configs['decon-first']['decon']['executable_path'] = "decon"

        if 'xy-res' in configs['decon-first']['decon']:
            if not type(configs['decon-first']['decon']['xy-res']) is float:
                exit('ERROR: decon-first decon xy resolution \'%s\' in config.json is not a float' % configs['decon-first']['decon']['xy-res'])
        else:
            print('WARNING: Using default MOSAIC pixel size of 0.108 um/pixel for deconvolution before deskewing')
            configs['decon-first']['decon']['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        configs['decon-first']['decon']['xy-res'] = {'flag': '-x', 'arg': configs['decon-first']['decon']['xy-res']}

        if 'n' in configs['decon-first']['decon']:
            if not type(configs['decon-first']['decon']['n']) is int:
                exit('ERROR: decon-first decon iteration number (n) \'%s\' in config.json must be 8, 16, or 32' % configs['decon-first']['decon']['n'])
            configs['decon-first']['decon']['n'] = {'flag': '-n', 'arg': configs['decon-first']['decon']['n']}
        
        if 'bit-depth' in configs['decon-first']['decon']:
            if configs['decon-first']['decon']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: decon-first decon bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['decon-first']['decon']['bit-depth'])
            configs['decon-first']['decon']['bit-depth'] = {'flag': '-b', 'arg': configs['decon-first']['decon']['bit-depth']}

        if 'subtract' in configs['decon-first']['decon']:
            if not type(configs['decon-first']['decon']['subtract']) is float:
                exit('ERROR: decon-first decon subtract value \'%s\' in config.json must be a float' % configs['decon-first']['decon']['subtract'])
            configs['decon-first']['decon']['subtract'] = {'flag': '-s', 'arg': configs['decon-first']['decon']['subtract']}

        if 'psf' not in configs['paths']:
            exit('ERROR: decon-first enabled, but no psf parameters found in config file')

        if 'dir' in configs['paths']['psf']:
            partial_path = root / configs['paths']['psf']['dir']
        else:
            configs['paths']['psf']['dir'] = None
            partial_path = root
            print('WARNING: no psf directory provided... using \'%s\'' % root)
        
        if 'resampled' not in configs['paths']['psf']:
            exit('ERROR: no resampled psf files provided for decon-first')

        for key, val in configs['paths']['psf']['resampled'].items():
            p = partial_path / val
            if not p.is_file():
                exit('ERROR: resampled %s psf file \'%s\' does not exist' % (key, p))

    # sanitize deskew configs
    if 'deskew' in configs:
        supported_opts = ['xy-res', 'fill', 'bit-depth', 'angle', 'executable_path']
        for key in list(configs['deskew']):
            if key not in supported_opts:
                print('WARNING: deskew option \'%s\' in config.json is not supported' % key)
                del configs['deskew'][key]

        if not 'executable_path' in configs['deskew']:
            configs['deskew']['executable_path'] = "deskew"

        if 'angle' in configs['deskew']:
            if not type(configs['deskew']['angle']) is float:
                exit('ERROR: deskew angle \'%s\' in config.json is not a float' % configs['deskew']['angle'])
        else:
            configs['deskew']['angle'] = 147.55 # Angle is 31.8 in LLSM but -32.45 for MOSAIC
            print('WARNING: Using default MOSAIC angle of 147.55 for deskewing')
        configs['deskew']['angle'] = {'flag': '-a', 'arg': configs['deskew']['angle']}

        if 'xy-res' in configs['deskew']:
            if not type(configs['deskew']['xy-res']) is float:
                exit('ERROR: deskew xy resolution \'%s\' in config.json is not a float' % configs['deskew']['xy-res'])
        else:
            print('WARNING: Using default MOSAIC pixel size of 0.108 um/pixel for deskewing')
            configs['deskew']['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        configs['deskew']['xy-res'] = {'flag': '-x', 'arg': configs['deskew']['xy-res']}
        
        if 'fill' in configs['deskew']:
            if not type(configs['deskew']['fill']) is float:
                exit('ERROR: deskew background fill value \'%s\' in config.json is not a float' % configs['deskew']['fill'])
            configs['deskew']['fill'] = {'flag': '-f', 'arg': configs['deskew']['fill']}

        if 'bit-depth' in configs['deskew']:
            if configs['deskew']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: deskew bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['deskew']['bit-depth'])
            configs['deskew']['bit-depth'] = {'flag': '-b', 'arg': configs['deskew']['bit-depth']}

    # sanitize decon configs
    if 'decon' in configs:
        supported_opts = ['xy-res','n', 'bit-depth', 'subtract', 'executable_path']
        for key in list(configs['decon']):
            if key not in supported_opts:
                print('WARNING: decon option \'%s\' in config.json is not supported' % key)
                del configs['decon'][key]

        if not 'executable_path' in configs['decon']:
            configs['decon']['executable_path'] = "decon"

        if 'xy-res' in configs['decon']:
            if not type(configs['decon']['xy-res']) is float:
                exit('ERROR: decon xy resolution \'%s\' in config.json is not a float' % configs['decon']['xy-res'])
        else:
            print('WARNING: Using default MOSAIC pixel size of 0.108 um/pixel for deconvolution')
            configs['decon']['xy-res'] = 0.108 # 0.108 um/pixel on the MOSAIC
        configs['decon']['xy-res'] = {'flag': '-x', 'arg': configs['decon']['xy-res']}

        if 'n' in configs['decon']:
            if not type(configs['decon']['n']) is int:
                exit('ERROR: decon iteration number (n) \'%s\' in config.json must be 8, 16, or 32' % configs['decon']['n'])
            configs['decon']['n'] = {'flag': '-n', 'arg': configs['decon']['n']}
        
        if 'bit-depth' in configs['decon']:
            if configs['decon']['bit-depth'] not in [8, 16, 32]:
                exit('ERROR: decon bit-depth \'%s\' in config.json must be 8, 16, or 32' % configs['decon']['bit-depth'])
            configs['decon']['bit-depth'] = {'flag': '-b', 'arg': configs['decon']['bit-depth']}

        if 'subtract' in configs['decon']:
            if not type(configs['decon']['subtract']) is float:
                exit('ERROR: decon subtract value \'%s\' in config.json must be a float' % configs['decon']['subtract'])
            configs['decon']['subtract'] = {'flag': '-s', 'arg': configs['decon']['subtract']}

        if 'psf' not in configs['paths']:
            exit('ERROR: decon enabled, but no psf parameters found in config file')

        if 'dir' in configs['paths']['psf']:
            partial_path = root / configs['paths']['psf']['dir']
        else:
            configs['paths']['psf']['dir'] = None
            partial_path = root
            print('WARNING: no psf directory provided... using \'%s\'' % root)
        
        if 'laser' not in configs['paths']['psf']:
            exit('ERROR: no psf files provided')

        for key, val in configs['paths']['psf']['laser'].items():
            p = partial_path / val
            if not p.is_file():
                exit('ERROR: laser %s psf file \'%s\' does not exist' % (key, p))

    # sanitize mip configs
    if 'mip' in configs:
        supported_opts = ['x', 'y', 'z', 'executable_path']
        for key in list(configs['mip']):
            if key not in supported_opts:
                print('WARNING: mip option \'%s\' in config.json is not supported' % key)
                del configs['mip'][key]

        if not 'executable_path' in configs['mip']:
            configs['mip']['executable_path'] = "mip"

        if 'x' in configs['mip']:
            if not type(configs['mip']['x']) is bool:
                exit('ERROR: mip x projection \'%s\' in config.json is not a true or false' % configs['mip']['x'])
            if configs['mip']['x']:
                configs['mip']['x'] = {'flag': '-x'}
            else:
                del configs['mip']['x']
        
        if 'y' in configs['mip']:
            if not type(configs['mip']['y']) is bool:
                exit('ERROR: mip y projection \'%s\' in config.json is not a true or false' % configs['mip']['y'])
            if configs['mip']['y']:
                configs['mip']['y'] = {'flag': '-y'}
            else:
                del configs['mip']['y']

        if 'z' in configs['mip']:
            if not type(configs['mip']['z']) is bool:
                exit('ERROR: mip z projection \'%s\' in config.json is not a true or false' % configs['mip']['z'])
            if configs['mip']['z']:
                configs['mip']['z'] = {'flag': '-z'}
            else:
                del configs['mip']['z']

    # sanitize bdv configs
    # (config flag for saving the output files in a BigDataViewer-compatible scheme)
    if 'bdv' in configs:
        supported_opts = ['bdv_save']
        for key in list(configs['bdv']):
            if key not in supported_opts:
                print('WARNING: bdv option \'%s\' in config.json is not supported' % key)
                del configs['bdv'][key]
        if 'bdv_save' in configs['bdv']:
            if not type(configs['bdv']['bdv_save']) is bool:
                exit('ERROR: bdv_save flag \'%s\' in config.json is not a true or false' % configs['bdv']['bdv_save'])                

    return configs

def get_processed_json(path):
    if path.is_file():
        with path.open(mode='r') as f:
            try:
                processed = json.load(f)
            except json.JSONDecodeError as e:
                exit('ERROR: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
        
        return processed
    else:
        return {}

def get_dirs(path, excludes):
    unprocessed_dirs = []

    # for root, dirs, files in os.walk(path):
    #     root = Path(root)

    #     # update directories to prevent us from traversing processed data
    #     dirs[:] = [d for d in dirs if str(root/d) not in excludes]

    #     # check if current dir contains a Settings.txt file
    #     for f in files:
    #         if f.endswith('Settings.txt'):
    #             # print(root)
    #             unprocessed_dirs.append(root)
    #             break

    # Manually check the upper level path
    files = os.listdir(path)
    # check if current dir contains a Settings.txt file
    for f in files:
        if f.endswith('Settings.txt'):
            unprocessed_dirs.append(path)
            break

    # Now walk the rest of the path
    for root, dirs, files in os.walk(path,topdown=False):
        root = Path(root)

        # update directories to prevent us from traversing processed data
        dirs[:] = [d for d in dirs if str(root / d) not in excludes]

        # check the good directories for settings files
        for d in dirs:
            files = os.listdir(root/d)
            # check if current dir contains a Settings.txt file
            for f in files:
                if f.endswith('Settings.txt'):
                    unprocessed_dirs.append(root/d)
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

# split strings at desired phrases (CamA, ch, t, etc.) to grab filename info
# clunky but it works
def string_finder(old_str, old_phrases):
    vars = []
    for str in old_phrases:
        new_str = old_str.partition(str)[2]
        new_str1 = new_str.partition('_')[0]
        vars.append(new_str1)
    return dict(zip(old_phrases,vars))

def process(dirs, configs, dryrun=False, verbose=False):
    processed = {}
    params_bsub = {
        'J': {
            'flag': '-J',
            'arg': 'mosaic-pipeline'
        },
        'o': {
            'flag': '-o',
            'arg': '/dev/null'
        },
        'We': {
            'flag': '-We',
            'arg': 250 # Was 10 for LLSM, 250 min > 4 h, keeps this from being put on a short queue that will kick it off too soon
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
    params_bdv = {}
    params_flatfield = {}
    params_deconfirst_decon = {}
    params_deconfirst_deskew = {}

    cmd_bsub = None
    cmd_crop = None
    cmd_flatfield = None
    cmd_deconfirst_decon = None
    cmd_deconfirst_deskew = None
    cmd_deskew = None
    cmd_decon = None
    cmd_mip = None

    # update default params with user defined configs and build commands
    if 'bsub' in configs:
        params_bsub.update(configs['bsub'])
        cmd_bsub = params2cmd(params_bsub, 'bsub')
    if 'flatfield' in configs:
        params_flatfield.update(configs['flatfield'])
        cmd_flatfield = params2cmd(params_flatfield,configs['flatfield']['executable_path'])
    if 'crop' in configs:
        params_crop.update(configs['crop'])
        cmd_crop = params2cmd(params_crop, configs['crop']['executable_path'])
    if 'decon-first'in configs:
        params_deconfirst_decon.update(configs['decon-first']['decon'])
        cmd_deconfirst_decon = params2cmd(params_deconfirst_decon, configs['decon-first']['decon']['executable_path'])
        params_deconfirst_deskew.update(configs['decon-first']['deskew'])
        cmd_deconfirst_deskew = params2cmd(params_deconfirst_deskew, configs['decon-first']['deskew']['executable_path'])
    if 'deskew' in configs:
        params_deskew.update(configs['deskew'])
        cmd_deskew = params2cmd(params_deskew, configs['deskew']['executable_path'])
    if 'decon' in configs:
        params_decon.update(configs['decon'])
        cmd_decon = params2cmd(params_decon, configs['decon']['executable_path'])
    if 'mip' in configs:
        params_mip.update(configs['mip'])
        cmd_mip = params2cmd(params_mip, configs['mip']['executable_path'])
    if 'bdv' in configs:
        params_bdv.update(configs['bdv'])

    # parse PSF settings files
    if 'decon' in configs:
        root = Path(configs['paths']['root'])

        psf_settings = {}
        for laser, psf_filename in configs['paths']['psf']['laser'].items():
            psf_settings[laser] = {}
            psf_filename, _ = os.path.splitext(psf_filename)
            p = root / configs['paths']['psf']['dir'] / (psf_filename + '_Settings.txt')
            if not p.is_file():
                exit('ERROR: laser %s psf file \'%s\' does not have a Settings file' % (laser, p))
   
            j = mosaicsettings2json.parse_txt(p)

            if j['waveform']['z-motion'] == 'X stage':
                exit('ERROR: PSF is skewed and cannot be used for decon')
            elif j['waveform']['z-motion'] == 'Z galvo & DO XZ stage':
                psf_settings[laser]['z-step'] = j['waveform']['xz-stage-offset']['interval'][0] 
            else:
                exit('ERROR: PSF z-motion cannot be determined for laser %s' % laser)
    if 'decon-first' in configs:
        root = Path(configs['paths']['root'])

        psf_resample_settings = {}
        for laser, psf_filename in configs['paths']['psf']['resampled'].items():
            psf_resample_settings[laser] = {}
            psf_filename, _ = os.path.splitext(psf_filename)
            p = root / configs['paths']['psf']['dir'] / (psf_filename + '_Settings.txt')
            if not p.is_file():
                exit('ERROR: resampled %s psf file \'%s\' does not have a Settings file' % (laser, p))
   
            j = mosaicsettings2json.parse_txt(p)

            if j['waveform']['z-motion'] == 'X stage':
                exit('ERROR: PSF is skewed and cannot be used for decon')
            elif j['waveform']['z-motion'] == 'Z galvo & DO XZ stage':
                psf_resample_settings[laser]['z-step'] = j['waveform']['xz-stage-offset']['interval'][0] 
            else:
                exit('ERROR: PSF z-motion cannot be determined for resampled %s' % laser)


    # process each directory
    for d in dirs:
        print('processing \'%s\'...' % d)

        # get list of files
        files = os.listdir(d)

        # parse the last Settings.txt file
        for f in reversed(files):
            if f.endswith('Settings.txt'):
                print('parsing \'%s\'...' % f)
                settings = mosaicsettings2json.parse_txt(d / f)
                #pattern = re.compile(f.split('_')[0] + '.*_ch(\d+).*\.tif') # Works unless using simultaneous acquisition
                pattern = re.compile(f.split('_')[0] + '.*_((CamA_ch(\d+))|(CamB_ch(\d+))).*\.tif')
                break

        # check for z-motion settings
        if 'waveform' not in settings:
            exit('ERROR: settings file did not contain a Waveform section')
        if 'z-motion' not in settings['waveform']:
            exit('ERROR: settings file did not contain a Z motion field')

        # save settings json
        if not dryrun:
            with open(d / 'settings.json', 'w') as path:
                json.dump(settings, path, indent=4)

        # bdv_file setup
        bdv_file = False
        attributes = []
        if params_bdv:
            print('saving in bdv naming format...')
            bdv_file = True
        for f in files:
            bdv_pattern = re.compile('scan_Cam(\d+)_ch(\d+)_tile(\d+)_t(\d+).*\.tif')
            m = bdv_pattern.fullmatch(f)
            if m:
                scan_type = 'bdv'
                attributes = [r'Cam_',r'ch_',r't_']
                pattern = pattern = re.compile(f.split('_')[0] + '.*_((Cam0_ch(\d+))|(Cam1_ch(\d+))).*\.tif')
                break
            m = pattern.fullmatch(f)
            if m:
                temp = re.findall(r'Iter',f)
                if bool(temp):
                    scan_type = 'tile'
                    attributes = [r'Cam',r'ch',r'Iter_']
                    break                
                scan_type = 'scan' # if not tile or bdv, general case
                attributes = [r'Cam',r'ch',r'stack']
        print('scan type is ' + scan_type)

        # flatfield setup
        flatfield = False
        if params_flatfield:
            if 'laser' not in settings['waveform']:
                exit('ERROR: settings file did not contain a Laser field')

            if 'x-stage-offset' in settings['waveform']:
                stepFlatfield = settings['waveform']['x-stage-offset']['interval']
            elif 'xz-stage-offset' in settings['waveform']:
                stepFlatfield = settings['waveform']['xz-stage-offset']['interval']
            else:
                exit('ERROR: settings file did not contain an valid step parameter for flatfield')

            flatfield = True

            root = Path(configs['paths']['root'])
            flatpaths = []
            for laser in settings['waveform']['laser']:
                filename = configs['paths']['flatfield']['laser'][str(laser)]
                flatpaths.append(root / configs['paths']['flatfield']['dir'] / filename )

            # flatfield output directory
            output_flatfield = d / 'flatfield'
            if not dryrun:
                output_flatfield.mkdir(exist_ok=True)

        # crop setup
        crop = False
        if params_crop:
            if 'x-stage-offset' in settings['waveform']:
                stepCrop = settings['waveform']['x-stage-offset']['interval']
            elif 'xz-stage-offset' in settings['waveform']:
                stepCrop = settings['waveform']['xz-stage-offset']['interval']
            else:
                exit('ERROR: settings file did not contain an valid step parameter for cropping')

            crop = True

            # crop output directory
            output_crop = d / 'crop'
            if not dryrun:
                output_crop.mkdir(exist_ok=True)

        # decon-first set up        
        deconfirst = False
        if params_deconfirst_deskew:

            deconfirst = True

            # deskew setup
            if 'x-stage-offset' not in settings['waveform']:
                exit('ERROR: settings file did not contain a X Stage Offset field')
            if 'interval' not in settings['waveform']['x-stage-offset']:
                exit('ERROR: settings file did not contain a X Stage Offset Interval field')           

            # get step sizes (This could be repeated if also doing regular deskew...)
            steps = settings['waveform']['x-stage-offset']['interval']

            # deskew output directory
            output_deconfirst_deskew = d / 'deskew_after_decon'
            if not dryrun:
                output_deconfirst_deskew.mkdir(exist_ok=True)

            # decon setup
            if 'laser' not in settings['waveform']:
                exit('ERROR: settings file did not contain a Laser field')

            root = Path(configs['paths']['root'])
            resamplepaths = []
            resamplesteps = []
            for laser in settings['waveform']['laser']:
                filename = configs['paths']['psf']['resampled'][str(laser)]
                resamplepaths.append(root / configs['paths']['psf']['dir'] / filename )
                resamplesteps.append(psf_resample_settings[str(laser)]['z-step'])

            # decon output directory
            output_deconfirst_decon = d / 'decon_before_deskew'
            if not dryrun:
                output_deconfirst_decon.mkdir(exist_ok=True)

        # deskew setup
        deskew = False
        if params_deskew:
            if 'x-stage-offset' not in settings['waveform']:
                exit('ERROR: settings file did not contain a X Stage Offset field')
            if 'interval' not in settings['waveform']['x-stage-offset']:
                exit('ERROR: settings file did not contain a X Stage Offset Interval field')

            deskew = True

            # get step sizes
            steps = settings['waveform']['x-stage-offset']['interval']

            # deskew output directory
            output_deskew = d / 'deskew'
            if not dryrun:
                output_deskew.mkdir(exist_ok=True)
        else:
            if not deconfirst:
                if settings['waveform']['z-motion'] == 'X stage':
                    print('WARNING: deskew is not enabled, but acquisition was stage scanned.')

        # decon setup
        decon = False
        if params_decon:
            if 'laser' not in settings['waveform']:
                exit('ERROR: settings file did not contain a Laser field')

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
            if deconfirst:
                output_deconfirst_mip = output_mip / 'deskew_after_decon'
                if not dryrun:
                    output_deconfirst_mip.mkdir(parents=True, exist_ok=True)
            if (not deskew) and (not deconfirst): # In this case, want mips of the 'original' data before decon
                if crop:
                    output_crop_mip = output_mip / 'crop'
                    if not dryrun:
                        output_crop_mip.mkdir(parents=True, exist_ok=True)
                elif flatfield:
                    output_flatfield_mip = output_mip / 'flatfield'
                    if not dryrun:
                        output_flatfield_mip.mkdir(parents=True, exist_ok=True)
                else:
                    output_original_mip = output_mip / 'original'
                    if not dryrun:
                        output_original_mip.mkdir(parents=True, exist_ok=True)

        # sort unique channels to deal with simultaneous acquisition file naming conventions
        # very clunky approach at the moment to deal with potentially awkward acquisition choices
        chList = []
        # parsing tile naming
        tiles = []
        pattern_tile = re.compile(r'(\_(?:[-]*\d{1,}?){1}x\_(?:[-]*\d{1,}?){1}y\_(?:[-]*\d{1,}?)z_)')
        if bdv_file:
            if scan_type == 'bdv': # In this case, we have a different file structure
                pattern_tile = re.compile(f.split('_')[0] + '.*_(tile(\d+)).*\.tif')

        for f in files:
            # construct a list of the channels
            m = pattern.fullmatch(f)
            if m:
                if m.group(1) not in chList:
                    chList.append(m.group(1))
            # construct list of tile names
            mm = re.findall(pattern_tile,f)
            if mm:
                if scan_type == 'bdv':
                    mm[0] = mm[0][0]
                if mm[0] not in tiles:
                    tiles.append(mm[0])
        tiles_dict = {tiles[i]:('_tile'+str(i)) for i in range(len(tiles))}

        sortVals = list(chList) 
        N_ch_CamA = 0
        for idx, name in enumerate(chList):
            # count number of CamA channels to offset CamB channel numbers
            temp = re.findall(r'CamA',name)
            if bool(temp):
                N_ch_CamA += 1
            sortVals[idx] = name[-1] + name[3]  # Names are sorted by camera, ch, but we want ch, camera sorting
        sortVals = sorted(sortVals)
        chNum = list(chList)
        for idx, name in enumerate(sortVals):
            chList[idx] = 'Cam' + name[1] + '_ch'+ name[0] # Recreate the names with the sorted values
            if scan_type == 'bdv':
                chNum[idx] = idx
            else:
                chNum[idx] = int(name[0])
        configs['parsing'] = {}
        configs['bdv_parsing'] = {}
        configs['bdv_parsing']['tile_names'] = tiles_dict
        param_parsing = {}
        param_bdv_parsing = {}
        chUnique = []
        for c in chNum:
            if c not in chUnique:
                chUnique.append(c)
        laserUse = list(chNum)
        for c in chUnique:
            lasersC = []
            nameC = []
            for idx, ch in enumerate(settings['waveform']['ch']):
                if ch == c:
                    lasersC.append(idx)
            for idx, ch in enumerate(chNum):
                if ch == c:
                    nameC.append(idx)              
            if len(lasersC) == 1:
                # In this case, map all the names to the single laser
                # Arbitrarily use CamB_ch += N_ch_CamA to avoid same channel names
                # The new naming scheme is stored in processed_config (use --verbose)
                chName = []
                for n in nameC:
                    laserUse[n] = lasersC[0]
                    print(chList[n] + '=' + str(settings['waveform']['laser'][lasersC[0]]))
                    chName.append(chList[n])
                configs['parsing'][settings['waveform']['laser'][lasersC[0]]] = chName
                temp = re.findall(r'CamA', chList[n])
                if bool(temp):
                    configs['bdv_parsing'][settings['waveform']['laser'][lasersC[0]]] = re.sub(r'CamA',r'Cam0',chList[n])
                else:
                    configs['bdv_parsing'][settings['waveform']['laser'][lasersC[0]]] = re.sub(r'CamB',r'Cam1',chList[n][0:-1]+str(int(chList[n][-1])+N_ch_CamA))
            elif len(lasersC) == len(nameC):
                # In this case, map the names to the lasers in order
                for idx, n in enumerate(nameC):
                    laserUse[n] = lasersC[idx]
                    print(chList[n] + '=' + str(settings['waveform']['laser'][lasersC[idx]]))
                    configs['parsing'][settings['waveform']['laser'][lasersC[idx]]] = chList[n]
                    temp = re.findall(r"CamA", chList[n])
                    if bool(temp):
                        configs['bdv_parsing'][settings['waveform']['laser'][lasersC[idx]]] = re.sub(r'CamA',r'Cam0',chList[n])
                    else:
                        configs['bdv_parsing'][settings['waveform']['laser'][lasersC[idx]]] = re.sub(r'CamB',r'Cam1',chList[n][0:-1]+str(int(chList[n][-1])+N_ch_CamA))
            else:
                # Other combinations of MOSAIC settings are not yet expected
                exit('ERROR: Unexpected naming convention')
        param_parsing.update(configs['parsing'])
        param_parsing.update(configs['bdv_parsing'])

        # Skew the PSFs accordingly if necessary
        # Will be saved to the current directory so that if multiple experiments are being processed the skewed files don't have the wrong step
        if deconfirst:             
            # This is a little bit manual to put some guard rails on what is requested...
            a = 180-configs['decon-first']['deskew']['angle']['arg']
            x = configs['decon-first']['deskew']['xy-res']['arg']
            fill = configs['decon-first']['deskew']['fill']['arg']
            bitd = configs['decon-first']['deskew']['bit-depth']['arg']
            # Check if all steps are the same because it is unclear how to best keep track of channels vs lasers here
            stepCheck = settings['waveform']['x-stage-offset']['interval']
            for step in stepCheck:
                if step != stepCheck[0]:
                    exit("ERROR: Different stage steps for different channels for %s. PSF skewing is not able to parse the steps correctly." % (d)) 
            step = settings['waveform']['x-stage-offset']['interval'][0] 
            # Loop through deskew commands
            bsub_psf = 'bsub -n 1 -J psf-skew -o /dev/null -We 1 -W 30 "'
            for psffile in resamplepaths:
                skewedPSF = PurePath(psffile)
                skewedPSF = skewedPSF.stem + '_skewed' + skewedPSF.suffix     
                skewedPSF = d / skewedPSF       
                if skewedPSF.is_file():    
                    print('Skewed PSF already exists and will not be overwritten: %s' % (skewedPSF))
                else:
                    tmp = configs['decon-first']['deskew']['executable_path'] + ' -a %s -x %s -f %s -b %s -w -s %s -o %s %s "' % (a,x,fill,bitd,step,skewedPSF,psffile)
                    cmd = bsub_psf + tmp
                    if verbose:
                        print(cmd)
                    if not dryrun:
                        os.system(cmd)
            if not dryrun: # check that files exist after the jobs have run
                for psffile in resamplepaths:
                    skewedPSF = PurePath(psffile)
                    skewedPSF = skewedPSF.stem + '_skewed' + skewedPSF.suffix     
                    skewedPSF = d / skewedPSF   
                    if not skewedPSF.is_file():
                        # TODO: can this better track the job above rather than waiting a few times?
                        print('waiting on skewed PSF saving...')
                        sleep(30)
                        if not skewedPSF.is_file():
                            print('still waiting on skewed PSF saving...')
                            sleep(30)
                        if not skewedPSF.is_file():
                            print('still waiting on skewed PSF saving...')
                            sleep(30)
                        if not skewedPSF.is_file():
                            exit("ERROR: skewed PSF taking too long to save....")


        # process all files in directory
        for f in files:
            m = pattern.fullmatch(f) 
            if m:
                chLaser = chList.index(m.group(1)) # This is the relevant index for parameters based on the laser, i.e., PSFs
                chLaser = laserUse[chLaser] # Pull from the list above as some channels may share lasers, etc.
                if scan_type == 'bdv':
                    ch = int(chLaser)
                else:
                    ch = int(m.group(1)[-1]) # This is the relevant index for things like stage scanning, etc.
                cmd = [cmd_bsub, ' \"']

                # Read filename and convert the outpath name to BDV format
                # Use 'scan_CamX_chX_tileX_tXXXX.tif' as convention
                if bdv_file & (scan_type != 'bdv'):
                    # attributes = [r'Cam',r'ch',r'stack']
                    details = string_finder(f,attributes)
                    temp = re.findall(r'CamA',f)
                # find corresponding tile name
                    tile_temp = re.findall(pattern_tile, f)
                    if tile_temp[0] in tiles_dict:
                        tile = tiles_dict[tile_temp[0]]
                    # Keep tile_0 if not
                    else:
                        tile = '_tile0'

                    if bool(temp):
                        # dst = f'scan_Cam_'+re.sub('A','0',details['Cam'])+'_ch_'+details['ch']+tile+'_t_'+details['stack']+'.tif'
                        dst = f'scan_Cam'+re.sub('A','0',details[attributes[0]])+'_ch'+details[attributes[1]]+tile+'_t'+details[attributes[-1]]+'.tif'
                    else:
                        # dst = f'scan_Cam_'+re.sub('B','1',details['Cam'])+'_ch_'+str(int(details['ch'])+N_ch_CamA)+tile+'_t_'+details['stack']+'.tif'
                        dst = f'scan_Cam'+re.sub('B','1',details[attributes[0]])+'_ch'+str(int(details[attributes[1]])+N_ch_CamA)+tile+'_t'+details[attributes[-1]]+'.tif'
                    out_f = f'{dst}'
                    
                else:
                    out_f = f

                if flatfield:
                    inpath = d / f
                    outpath = output_flatfield / tag_filename(out_f, '_flatfield')
                    root = Path(configs['paths']['root'])
                    flatpath = root / configs['paths']['flatfield']['dir']
                    darkpath =  flatpath / configs['paths']['flatfield']['dark']
                    normpath = flatpaths[ch]
                    tmp = cmd_flatfield + ' -w -d %s -n %s -x %s -q %s -o %s  %s;' % (darkpath,normpath,configs['xy-res'],stepFlatfield[ch], outpath, inpath)
                    cmd.append(tmp)

                if crop:
                    if flatfield:
                        inpath = output_flatfield / tag_filename(out_f, '_flatfield')
                    else:
                        inpath = d / f
                    outpath = output_crop / tag_filename(out_f, '_crop')                    
                    tmp = cmd_crop + ' -w -s %s -o %s  %s;' % (stepCrop[ch], outpath, inpath)
                    cmd.append(tmp)

                if (not deskew) and (not deconfirst) and mip: # don't need all of all the mips if deskewing, so only create useful ones to save file space
                    #  get the appropriate spacing
                    if 'xz-stage-offset' in settings['waveform']:
                        step = settings['waveform']['xz-stage-offset']['interval'][ch]
                    else:
                        print('WARNING: deskew is not enabled, but MIPs are being prepared for input stage scanned images with default MOSAIC angle of 147.55.')
                        step = settings['waveform']['x-stage-offset']['interval'][ch] 
                        step = step * math.sin(abs(147.55* math.pi/180.0))                     

                    # prepare appropriate mips
                    if crop:
                        xyRes = configs['xy-res']
                        inpath = output_crop / tag_filename(out_f, '_crop')
                        outpath = output_crop_mip / tag_filename(out_f, '_crop_mip')
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes,step, outpath, inpath)
                        cmd.append(tmp)
                    elif flatfield:
                        xyRes = configs['xy-res']
                        inpath = output_flatfield / tag_filename(out_f, '_flatfield')
                        outpath = output_flatfield_mip / tag_filename(out_f, '_flatfield_mip')
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes,step, outpath, inpath)
                        cmd.append(tmp)
                    else:
                        xyRes = 0.108 # For no cropping, use default
                        inpath = d / f
                        outpath = output_original_mip / tag_filename(out_f, '_mip')
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes,step, outpath, inpath)
                        cmd.append(tmp)

                if deconfirst:

                    # Make sure that the skewed files exist
                    skewedPSF = PurePath(resamplepaths[chLaser])
                    skewedPSF = skewedPSF.stem + '_skewed' + skewedPSF.suffix     
                    skewedPSF = d / skewedPSF
                    if not dryrun:
                        if not skewedPSF.is_file():
                            exit('ERROR: skewed psf file \'%s\' does not exist' % (skewedPSF))

                    # Set up by getting the appropriate step size
                    step = settings['waveform']['x-stage-offset']['interval'][ch] 
                    step = step * math.sin(abs(configs['decon-first']['deskew']['angle']['arg']) * math.pi/180.0)                     
                        
                    # Which input image?
                    if crop:
                        inpath = output_crop / tag_filename(out_f, '_crop')
                    elif flatfield:
                        inpath = output_flatfield / tag_filename(out_f, '_flatfield')
                    else:
                        inpath = d / f

                    outpath = output_deconfirst_decon / tag_filename(out_f, '_decon_before_deskew')

                    # ASSUMPTION: the resampled PSF has a spacing equivalent to the image.
                    tmp = cmd_deconfirst_decon + ' -w -k %s -p %s -q %s -o %s %s;' % (skewedPSF, step, step, outpath, inpath)
                    cmd.append(tmp)

                    # Now work on deskewing
                    inpath = PurePath(outpath)
                    outpath = output_deconfirst_deskew / tag_filename(out_f, '_deskew_after_decon')

                    tmp = cmd_deconfirst_deskew + ' -w -s %s -o %s  %s;' % (steps[ch], outpath, inpath) # input is steps (not step) b/c angle calculations occur in deskew command
                    cmd.append(tmp)

                    # create mips for deskew_after_decon
                    if mip:
                        inpath = output_deconfirst_deskew / tag_filename(out_f, '_deskew_after_decon')
                        outpath = output_deconfirst_mip / tag_filename(out_f, '_deskew_after_decon_mip')
                        xyRes = configs['decon-first']['deskew']['xy-res']['arg']
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes, step, outpath, inpath)
                        cmd.append(tmp)                            

                if deskew:
                    if crop:
                        inpath = output_crop / tag_filename(out_f, '_crop')
                    elif flatfield:
                        inpath = output_flatfield / tag_filename(out_f, '_flatfield')
                    else:
                        inpath = d / f
                    outpath = output_deskew / tag_filename(out_f, '_deskew')
                    step = settings['waveform']['x-stage-offset']['interval'][ch] 
                    step = step * math.sin(abs(configs['deskew']['angle']['arg']) * math.pi/180.0) 

                    tmp = cmd_deskew + ' -w -s %s -o %s  %s;' % (steps[ch], outpath, inpath) # input is steps (not step) b/c angle calculations occur in deskew command
                    cmd.append(tmp)

                    # create mips for deskew
                    if mip:
                        inpath = output_deskew / tag_filename(out_f, '_deskew')
                        outpath = output_deskew_mip / tag_filename(out_f, '_deskew_mip')
                        xyRes = configs['deskew']['xy-res']['arg']
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes, step, outpath, inpath)
                        cmd.append(tmp)

                if decon:
                    if deskew:
                        inpath = output_deskew / tag_filename(out_f, '_deskew')
                    elif crop:
                        inpath = output_crop / tag_filename(out_f, '_crop')
                    elif flatfield:
                        inpath = output_flatfield / tag_filename(out_f, '_flatfield')
                    else:
                        inpath = d / f

                    outpath = output_decon / tag_filename(out_f, '_decon')

                    if deskew:
                        step = settings['waveform']['x-stage-offset']['interval'][ch] 
                        step = step * math.sin(abs(configs['deskew']['angle']['arg']) * math.pi/180.0) 
                    else:
                        #  get the appropriate spacing
                        if 'xz-stage-offset' in settings['waveform']:
                            step = settings['waveform']['xz-stage-offset']['interval'][ch]
                        else:
                            print('WARNING: deskew is not enabled, but the deconvolution will use the default MOSAIC angle of 147.55 for step size.')
                            step = settings['waveform']['x-stage-offset']['interval'][ch] 
                            step = step * math.sin(abs(147.55* math.pi/180.0)) 

                    # print(m.group(1),psfpaths[chLaser]) # uncomment this to check on the parsing
                    tmp = cmd_decon + ' -w -k %s -p %s -q %s -o %s %s;' % (psfpaths[chLaser], psfsteps[chLaser], step, outpath, inpath)
                    cmd.append(tmp)

                    # create mips for decon
                    if mip:
                        xyRes = configs['decon']['xy-res']['arg']
                        inpath = output_decon / tag_filename(out_f, '_decon')
                        outpath = output_decon_mip / tag_filename(out_f, '_decon_mip')
                        tmp = cmd_mip + ' -p %s -q %s -o %s %s;' % (xyRes,step, outpath , inpath)
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
        processed[d]['parsing'] = param_parsing
        processed[d]['bdv_parsing'] = param_bdv_parsing
        if flatfield:
            processed[d]['flatfield'] = params_flatfield
        if crop:
            processed[d]['crop'] = params_crop
        if deconfirst:
            processed[d]['decon-first'] = {}
            processed[d]['decon-first']['deskew'] = params_deconfirst_deskew
            processed[d]['decon-first']['decon'] = params_deconfirst_decon
        if deskew:
            processed[d]['deskew'] = params_deskew
            processed[d]['deskew']['step'] = steps
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
    # if args.dryrun:
    #     print(configs)

    # get dictionary of processed files
    root_dir = Path(configs['paths']['root'])
    processed_path = root_dir / 'processed.json'
    processed_json = get_processed_json(processed_path)

    # get all directories containing a Settings files
    excludes = list(processed_json)
    if 'psf' in configs['paths']:
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
