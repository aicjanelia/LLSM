#! /misc/local/python-3.8.2/bin/python3
"""
filename2json
This program converts a user-specified v4.04505.Development LLSM Settings file to a json.
"""

import argparse
import re
import json
from sys import exit

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Converts an LLSM filename into a parsed JSON')
    parser.add_argument('input', type=str, help='path to LLSM image file')
    args = parser.parse_args()

    if not str(args.input).lower().endswith(('.tif', '.tiff')):
        print('warning: \'%s\' does not appear to be a LLSM image file\n' % args.input)

    return args

"""
Adds a parsed string to a given dictionary
"""
def search_pattern(data, key, string, pattern, cast_as=str):
    m = re.search(pattern, string)

    if m:
        if cast_as == str:
            data[key] = m.group(1).strip()
        else:
            data[key] = cast_as(m.group(1).strip())
        
"""
Converter for v4.04505.Development LLSM TIFF files
"""
def convert(path):
    data = {}

    # parse user-specified name
    search_pattern(data, 'name', path, r'^Sample :\\t(.*)')

    # parse iter indices
    search_pattern(data, 'name', path, r'^Sample :\\t(.*)', cast_as=int)

    # parse camera
    search_pattern(data, 'camera', path, r'^Sample :\\t(.*)')

    # parse channel
    search_pattern(data, 'channel', path, r'^Sample :\\t(.*)', cast_as=int)

    # parse stack
    search_pattern(data, 'stack', path, r'^X :\t(\d*)', cast_as=int)

    # parse laser
    search_pattern(data, 'laser', path, r'^X :\t(\d*)', cast_as=int)

    # parse relative time
    search_pattern(data, 'time_rel', path, r'^X :\t(\d*)', cast_as=int)

    # parse absolute time
    search_pattern(data, 'time_abs', path, r'^X :\t(\d*)', cast_as=int)

    # parse x,y,z,t indices


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