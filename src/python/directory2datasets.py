#! /misc/local/python-3.8.2/bin/python3
"""
directory2datasets
This program represents a user-specified directory as an array of dataset objects.
"""

import argparse
import os
from pathlib import Path
from sys import exit

import utils
import dataset

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Converts a directory of LLSM files into a collection of datasets')
    parser.add_argument('input', type=Path, help='root directory path')
    args = parser.parse_args()

    if not args.input.is_dir():
        exit('error: \'%s\' does not exist' % args.input)

    return args

"""
Enumerates all directories containing a Settings.txt file
"""
def get_dataset_dirs(path, excludes):
    dataset_dirs = []

    for root, dirs, files in os.walk(path):
        root = Path(root)

        # update directories to prevent us from traversing processed data
        dirs[:] = [d for d in dirs if str(root / d) not in excludes]

        # check if root contains a Settings.txt file
        for f in files:
            if f.endswith('Settings.txt'):
                dataset_dirs.append(root)
                break

    return dataset_dirs

"""
Generates a collection of datasets from a defined root directory
"""
def convert(path, dryrun=False, verbose=False):
    # get dictionary of processed files (if it exists)
    processed = utils.load_json(path / 'processed.json', False)

    # get all directories containing a Settings files
    excludes = list(processed)
    dataset_dirs = get_dataset_dirs(path, excludes)

    datasets = []
    for d in dataset_dirs:
        datasets.append(dataset.Dataset(d))

    return datasets

"""
Command-line
"""
if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # generate datasets
    datasets = convert(args.input)

    for dataset in datasets:
        print(dataset)