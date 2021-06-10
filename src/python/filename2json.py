#! /misc/local/python-3.8.2/bin/python3
"""
filename2json
This program converts a user-specified v4.04505.Development LLSM Settings file to a json.
"""

import argparse
import re
import json
from sys import exit

import utils

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Converts an LLSM filename into a parsed JSON')
    parser.add_argument('input', type=str, help='path to LLSM image file')
    args = parser.parse_args()

    if not args.input.lower().endswith(('.tif', '.tiff')):
        print('warning: \'%s\' does not appear to be a LLSM image file\n' % args.input)

    return args
        
"""
Converter for v4.04505.Development LLSM TIFF files
"""
def convert(path):
    data = {}

    # parse user-specified name
    utils.find_first_pattern(data, 'name', path, r'^(.*?)_(?:Iter|Cam|ch)')

    # parse iter indices
    utils.find_every_pattern(data, 'iters', path, r'_Iter_(\d+)', cast_as=int)

    # parse camera
    utils.find_last_pattern(data, 'camera', path, r'_Cam([A-B])')

    # parse channel
    utils.find_last_pattern(data, 'channel', path, r'_ch(\d+)', cast_as=int)

    # parse stack
    utils.find_last_pattern(data, 'stack', path, r'_stack(\d+)', cast_as=int)

    # parse laser
    utils.find_last_pattern(data, 'laser', path, r'_(\d+)nm', cast_as=int)

    # parse relative time
    utils.find_last_pattern(data, 'time_rel', path, r'_(\d+)msec_', cast_as=int)

    # parse absolute time
    utils.find_last_pattern(data, 'time_abs', path, r'_(\d+)msecAbs', cast_as=int)

    # parse x,y,z,t indices
    utils.find_last_pattern(data, 'x', path, r'_(\d+)x', cast_as=int)
    utils.find_last_pattern(data, 'y', path, r'_(\d+)y', cast_as=int)
    utils.find_last_pattern(data, 'z', path, r'_(\d+)z', cast_as=int)
    utils.find_last_pattern(data, 't', path, r'_(\d+)t', cast_as=int)

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