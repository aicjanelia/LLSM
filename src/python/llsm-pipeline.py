#! /misc/local/python-3.8.2/bin/python3
"""
llsm-pipeline
This program is the entry point for the LLSM pipeline.
"""

import argparse
from pathlib import Path
from sys import exit

import config2pipeline

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Entry point for the LLSM pipeline.')
    parser.add_argument('input', type=Path, help='path to configuration JSON file')
    parser.add_argument('--dry-run', '-d', default=False, action='store_true', dest='dryrun',help='execute without creating/modifying files or submitting bsub jobs')
    parser.add_argument('--verbose', '-v', default=False, action='store_true', dest='verbose',help='print details (including commands to bsub)')
    args = parser.parse_args()

    if not args.input.is_file():
        exit(f'error: \'%s\' does not exist' % args.input)

    if not args.input.suffix == '.json':
        print(f'warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # create pipeline
    pipeline = config2pipeline.convert(args.input, args.verbose)

    # execute pipeline
    pipeline.execute(args.dryrun, args.verbose)

    print('Done')
