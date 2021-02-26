#! /misc/local/python-3.8.2/bin/python3
"""
llsm-pipeline
This program is the entry point for the LLSM pipeline.
"""

import argparse
import re
import json
from pathlib import Path
from sys import exit
import settings2json

def parse_args():
    parser = argparse.ArgumentParser(description='Batch deskewing and deconvolution script for LLSM images.')
    parser.add_argument('input', help='path to configuration JSON file')
    args = parser.parse_args()

    if not Path(args.input).is_file():
        exit(f'error: \'%s\' does not exist' % args.input)

    if not args.input.endswith('.json'):
        print(f'warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

def load_configs(path):
    with Path(path).open(mode='r') as f:
        try:
            configs = json.load(f)
        except json.JSONDecodeError as e:
            exit(f'error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))

    # check critical configs
    root = Path(configs["paths"]["root"])
    if not root.is_dir():
        exit(f'error: root path \'%s\' does not exist' % root)

    if 'decon' in configs:
        if 'psf' in configs['paths']:
            if 'dir' in configs['paths']['psf']:
                partial_path = root / configs['paths']['psf']['dir']
            else:
                configs['paths']['psf']['dir'] = None
                partial_path = root
                print(f'warning: no psf directory provided... using \'%s\'' % root)
            
            if 'laser' in configs['paths']['psf']:
                for key, val in configs['paths']['psf']['laser'].items():
                    p = partial_path / val
                    if not p.is_file():
                        exit(f'error: laser %s psf file \'%s\' does not exist' % (key, p))
            else:
                exit(f'error: no psf files provided')
        else:
            exit(f'error: decon enabled, but no psf parameters found in config file')

    return configs

def get_processed_json(path):
    if path.is_file():
        with path.open(mode='r') as f:
            try:
                processed = json.load(f)
            except json.JSONDecodeError as e:
                exit(f'error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
        
        return processed
    else:
        return {}

def get_dirs(path, excludes):
    unprocessed_dirs = []

    for root, dirs, files in os.walk(path):
        root = Path(root)

        # update directories to prevent us from traversing processed data
        dirs[:] = [d for d in dirs if (root / d) not in excludes]

        # check if root contains a Settings.txt file
        for f in files:
            if f.endswith('Settings.txt'):
                unprocessed_dirs.append(root)
                break

    return unprocessed_dirs


def process

if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # load config file
    configs = load_configs(args.input)

    # get dictionary of processed files
    root_dir = Path(configs['paths']['root'])
    processed_path = root_dir / 'processed.json'
    processed = get_processed_json(processed_path)

    # get all directories containing a Settings files
    excludes = list(processed)
    if configs['paths']['psf']['dir'] != None:
        excludes.append(root_dir / configs['paths']['psf']['dir'])

    unprocessed_dirs = get_dirs(root_dir, excludes)

    # process images in new diectories
    new_processed = process(unprocessed_dirs, configs)

    # update processed.json
    processed.update(new_processed)
    with Path(root_dir)

    print('Done')
