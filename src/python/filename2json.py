#! /misc/local/python-3.8.2/bin/python3
"""
filename2json
This program converts a user-specified v4.04505.Development LLSM Settings file to a json.
"""

import argparse
import re
import json
from pathlib import Path
from sys import exit

import utils

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Converts an LLSM filename into a parsed JSON')
    parser.add_argument('input', type=Path, help='path to LLSM image file')
    args = parser.parse_args()

    if not str(args.input).lower().endswith(('.tif', '.tiff')):
        print('warning: \'%s\' does not appear to be a LLSM image file\n' % args.input)

    return args
        
"""
Converter for a v4.04505.Development LLSM TIFF file
"""
def convert(path):
    data = {}

    # strip everything except filename
    filename = path.stem

    # parse user-specified name
    utils.find_first_pattern(data, 'name', filename, r'^(.*?)_(?:Iter|Cam|ch)')
    if 'name' not in data:
        exit('error: could not parse user-specified name field')

    # parse iter indices
    utils.find_every_pattern(data, 'iters', filename, r'_Iter_(\d+)', cast_as=int)
    if 'iters' in data:
        data['is_macro'] = True
    else:
        data['is_macro'] = False

    # parse camera
    utils.find_last_pattern(data, 'camera', filename, r'_Cam([A-B])')
    if 'camera' in data:
        data['is_dualcam'] = True
    else:
        data['is_dualcam'] = False

    # parse channel
    utils.find_last_pattern(data, 'channel', filename, r'_ch(\d+)', cast_as=int)
    if 'channel' not in data:
        exit('error: could not parse channel field')

    # parse stack
    utils.find_last_pattern(data, 'stack', filename, r'_stack(\d+)', cast_as=int)
    if 'stack' not in data:
        exit('error: could not parse stack field')

    # parse laser
    utils.find_last_pattern(data, 'laser', filename, r'_(\d+)nm', cast_as=int)
    if 'laser' not in data:
        exit('error: could not parse laser field')

    # parse relative time
    utils.find_last_pattern(data, 'time_rel', filename, r'_(\d+)msec_', cast_as=int)
    if 'time_rel' not in data:
        exit('error: could not parse relative time field')

    # parse absolute time
    utils.find_last_pattern(data, 'time_abs', filename, r'_(\d+)msecAbs', cast_as=int)
    if 'time_abs' not in data:
        exit('error: could not parse absolute time field')

    # parse x,y,z,t indices
    utils.find_last_pattern(data, 'x', filename, r'_(\d+)x', cast_as=int)
    utils.find_last_pattern(data, 'y', filename, r'_(\d+)y', cast_as=int)
    utils.find_last_pattern(data, 'z', filename, r'_(\d+)z', cast_as=int)
    utils.find_last_pattern(data, 't', filename, r'_(\d+)t', cast_as=int)
    if 'x' not in data:
        exit('error: could not parse x index field')
    if 'y' not in data:
        exit('error: could not parse y index field')
    if 'z' not in data:
        exit('error: could not parse z index field')
    if 't' not in data:
        exit('error: could not parse t index field')

    return data

"""
Command-line
"""
if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # parse filename
    data = convert(args.input)

    print(json.dumps(data, indent=4))