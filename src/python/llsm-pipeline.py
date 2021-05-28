#! /misc/local/python-3.8.2/bin/python3
"""
llsm-pipeline
This program is the entry point for the LLSM pipeline.
"""

import argparse
import json
from pathlib import Path
from sys import exit

import config2pipeline

def parse_args():
    parser = argparse.ArgumentParser(description='Batch deskewing and deconvolution script for LLSM images.')
    parser.add_argument('input', type=Path, help='path to configuration JSON file')
    parser.add_argument('--dry-run', '-d', default=False, action='store_true', dest='dryrun',help='execute without submitting any bsub jobs')
    parser.add_argument('--verbose', '-v', default=False, action='store_true', dest='verbose',help='print details (including commands to bsub)')
    args = parser.parse_args()

    if not args.input.is_file():
        exit(f'error: \'%s\' does not exist' % args.input)

    if not args.input.suffix == '.json':
        print(f'warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

def load_json(path):
    if path.is_file():
        with path.open(mode='r') as f:
            try:
                j = json.load(f)
            except json.JSONDecodeError as e:
                exit(f'error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
        
        return j
    else:
        return {}

if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # load config file
    configs = load_json(args.input)
    pipeline = config2pipeline.convert(configs)

    # get dictionary of processed files (if it exists)
    root_dir = Path(configs['dataset']['root'])
    processed_path = root_dir / 'processed.json'
    processed_json = load_json(processed_path)

    # get all directories containing a Settings files
    excludes = list(processed_json)
    if configs['dataset']['psf']['dir'] != None:
        excludes.append(str(root_dir / configs['dataset']['psf']['dir']))

    unprocessed_dirs = get_dirs(root_dir, excludes)

    # process images in new directories
    processed_dirs = graph_processor.process(unprocessed_dirs, configs, dryrun=args.dryrun, verbose=args.verbose)

    # update processed.json
    processed_json.update(processed_dirs)
    if not args.dryrun:
        with processed_path.open(mode='w') as f:
            f.write(json.dumps(processed_json, indent=4))

    if args.verbose:
        print('processed.json...')
        print(json.dumps(processed_json, indent=4))

    print('Done')
