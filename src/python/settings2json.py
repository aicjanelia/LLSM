#! /misc/local/python-3.8.2/bin/python3

import argparse
import re
import json
from pathlib import Path
from sys import exit

def parse_args():
    parser = argparse.ArgumentParser(description='Converts an LLSM Settings.txt file into a parsed JSON')
    parser.add_argument('input', help='path to Settings.txt file')
    args = parser.parse_args()

    if not Path(args.input).exists():
        exit(f'error: \'%s\' does not exist' % args.input)

    if not args.input.endswith('Settings.txt'):
        print(f'warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

def search_pattern(data, key, string, pattern, cast_as=str):
    m = re.search(pattern, string, re.MULTILINE)

    if m:
        if cast_as == str:
            data[key] = m.group(1).strip()
        else:
            data[key] = cast_as(m.group(1).strip())
        

"""
Parser for v4.04505.Development Settings files
"""
def parse_txt(path):
    data = {}

    # read file content to memory
    path = Path(path)
    f = path.open(mode='r')
    txt = f.read()
    f.close()

    # split by section (e.g., ***** ***** ***** General ***** ***** *****)
    sections = re.split(r'\*{5} \*{5} \*{5} +(\S* ??\S*) +\*{5} \*{5} \*{5}', txt)
    
    # parse General
    try:
        idx = sections.index('General') + 1
    except ValueError:
        print('warning: cannot find General section\n')
    else:
        sctn = 'general'
        data[sctn] = {}

        search_pattern(data[sctn], 'date', sections[idx], r'^Date :\t(.*)')
        search_pattern(data[sctn], 'acq-mode', sections[idx], r'^Acq Mode :\t(.*)')
        search_pattern(data[sctn], 'version', sections[idx], r'^Version :\t(.*)')
        search_pattern(data[sctn], 'pc', sections[idx], r'^PC :\t(.*)')
    
    # parse Notes
    try:
        idx = sections.index('Notes') + 1
    except ValueError:
        print('warning: cannot find Notes section\n')
    else:
        sctn = 'notes'
        data[sctn] = {}

        search_pattern(data[sctn], 'microscopist', sections[idx], r'^Microscopist :\\t(.*)')
        search_pattern(data[sctn], 'collaborator', sections[idx], r'^Collaborator :\\t(.*)')
        search_pattern(data[sctn], 'sample', sections[idx], r'^Sample :\\t(.*)')
        search_pattern(data[sctn], 'notes', sections[idx], r'Notes :\\t([\w\W]*)')

    # parse 3D Tiling
    try:
        idx = sections.index('3D Tiling') + 1
    except ValueError:
        print('warning: cannot find 3D Tiling section\n')
    else:
        sctn = '3d-tiling'
        data[sctn] = {}

        search_pattern(data[sctn], 'x', sections[idx], r'^X :\t(\d*)', cast_as=int)
        search_pattern(data[sctn], 'y', sections[idx], r'^Y :\t(\d*)', cast_as=int)
        search_pattern(data[sctn], 'z', sections[idx], r'^Z :\t(\d*)', cast_as=int)
        search_pattern(data[sctn], 't', sections[idx], r'^T :\t(\d*)', cast_as=int)

    # parse Waveform
    try:
        idx = sections.index('Waveform') + 1
    except ValueError:
        print('warning: cannot find Waveform section\n')
    else:
        sctn = 'waveform'
        data[sctn] = {}

        search_pattern(data[sctn], 'waveform-type', sections[idx], r'^Waveform type :\t(.*)')
        search_pattern(data[sctn], 'cycle-lasers', sections[idx], r'^Cycle lasers :\t(.*)')
        search_pattern(data[sctn], 'z-motion', sections[idx], r'^Z motion :\t(.*)')

    return json.dumps(data, sort_keys=False, indent=4)

def main():
    # get command line arguments
    args = parse_args()

    # parse Settings.txt file
    settings_json = parse_txt(args.input)

    print(settings_json)

if __name__ == '__main__':
    main()