#! /misc/local/python-3.8.2/bin/python3

import argparse

def parse_args():
    parser = argparse.ArgumentParser(description='Converts an LLSM Settings.txt file into a parsed JSON')
    parser.add_argument('--input', '-i', dest='path', help='path to Settings.txt file')
    args = parser.parse_args()

    return args

def main():
    args = parse_args()

    return False

if __name__ == '__main__':
    main()