#! /misc/local/python-3.8.2/bin/python3

import argparse
import json
from pathlib import Path
from sys import exit

"""
Parser for command line arguments
"""
def parse_args():
    parser = argparse.ArgumentParser(description='Converts an config.json file into a pipeline')
    parser.add_argument('input', type=Path, help='path to config.json file')
    args = parser.parse_args()

    if not args.input.is_file():
        exit('error: \'%s\' does not exist' % args.input)

    if not str(args.input).endswith('config.json'):
        print('warning: \'%s\' does not appear to be a settings file\n' % args.input)

    return args

"""
Sanitizes user-defined deskew options from config.json
"""
def sanitize_deskew_opts(options):
    supported_opts = ['xy-res', 'fill', 'bit-depth']
    for key in list(options):
        if key not in supported_opts:
            print('warning: deskew option \'%s\' in config.json is not supported' % key)
            del options[key]

    if 'xy-res' in options:
        if type(options['xy-res']) is not float:
            exit('error: deskew xy resolution \'%s\' in config.json is not a float' % options['xy-res'])
        options['xy-res'] = {'flag': '-x', 'arg': options['xy-res']}
    
    if 'fill' in options:
        if type(options['fill']) is not float:
            exit('error: deskew background fill value \'%s\' in config.json is not a float' % options['fill'])
        options['fill'] = {'flag': '-f', 'arg': options['fill']}

    if 'bit-depth' in options:
        if options['bit-depth'] not in [8, 16, 32]:
            exit('error: deskew bit-depth \'%s\' in config.json must be 8, 16, or 32' % options['bit-depth'])
        options['bit-depth'] = {'flag': '-b', 'arg': options['bit-depth']}

    return options

"""
Sanitizes user-defined decon options from config.json
"""
def sanitize_decon_opts(options):
    supported_opts = ['n', 'bit-depth', 'subtract', 'psf']
    for key in list(options):
        if key not in supported_opts:
            print('warning: decon option \'%s\' in config.json is not supported' % key)
            del options[key]

    if 'n' in options:
        if type(options['n']) is not int:
            exit('error: decon iteration number (n) \'%s\' in config.json must be 8, 16, or 32' % options['n'])
        options['n'] = {'flag': '-n', 'arg': options['n']}
    
    if 'bit-depth' in options:
        if options['bit-depth'] not in [8, 16, 32]:
            exit('error: decon bit-depth \'%s\' in config.json must be 8, 16, or 32' % options['bit-depth'])
        options['bit-depth'] = {'flag': '-b', 'arg': options['bit-depth']}

    if 'subtract' in options:
        if type(options['subtract']) is not float:
            exit('error: decon subtract value \'%s\' in config.json must be a float' % options['subtract'])
        options['subtract'] = {'flag': '-s', 'arg': options['subtract']}

    if 'psf' not in options:
        exit('error: decon enabled, but no psf parameters found in config file')

    if 'path' in options['psf']:
        partial_path = root / options['psf']['path']
    else:
        options['psf']['path'] = None
        partial_path = root
        print('warning: no psf directory provided... using \'%s\'' % root)
    
    if 'laser' not in options['psf']:
        exit('error: no psf files provided')

    for key, val in options['psf']['laser'].items():
        p = partial_path / val
        if not p.is_file():
            exit('error: laser %s psf file \'%s\' does not exist' % (key, p))
    
    return options

"""
Sanitizes user-defined mip options from config.json
"""
def sanitize_mip_opts(options):
    supported_opts = ['x', 'y', 'z']
    for key in list(options):
        if key not in supported_opts:
            print('warning: mip option \'%s\' in config.json is not supported' % key)
            del options[key]

    if 'x' in options:
        if type(options['x']) is not bool:
            exit('error: mip x projection \'%s\' in config.json is not a true or false' % options['x'])
        if options['x']:
            options['x'] = {'flag': '-x'}
        else:
            del options['x']
    
    if 'y' in options:
        if type(options['y']) is not bool:
            exit('error: mip y projection \'%s\' in config.json is not a true or false' % options['y'])
        if options['y']:
            options['y'] = {'flag': '-y'}
        else:
            del options['y']

    if 'z' in options:
        if type(options['z']) is not bool:
            exit('error: mip z projection \'%s\' in config.json is not a true or false' % options['z'])
        if options['z']:
            options['z'] = {'flag': '-z'}
        else:
            del options['z']
    
    return options

"""
Sanitizes user-defined bsub options from config.json
"""
def sanitize_bsub_opts(options):
    supported_opts = ['o', 'We', 'n']
    for key in list(options):
        if key not in supported_opts:
            print('warning: bsub option \'%s\' in config.json is not supported' % key)
            del options[key]

    if 'o' in options:
        if not Path(options['o']).is_dir:
            exit('error: bsub output directory \'%s\' in config.json is not a valid path' % options['o'])
        options['o'] = {'flag': '-o', 'arg': options['o']}
    
    if 'We' in options:
        if type(options['We']) is not int:
            exit('error: bsub estimated wait time \'%s\' in config.json is not an integer' % options['We'])
        options['We'] = {'flag': '-We', 'arg': options['We']}
    
    if 'n' in options:
        if type(options['n']) is not int:
            exit('error: bsub slot count \'%s\' in config.json is not an integer' % options['n'])
        options['n'] = {'flag': '-n', 'arg': options['n']}
    
    return options

"""
Checks that each node is properly formatted
"""
def sanitize_node(configs, node_id):
    supported_modules = ['deskew',
                         'decon',
                         'mip']

    node = configs[node_id]

    if node_id == 'datasets':
        # sanitize root path
        root = Path(configs['datasets']['root'])
        if not root.is_dir():
            exit('error: root path \'%s\' does not exist' % root)
    else:
        # check parent
        if 'parent' not in node:
            exit('error: node \'%s\' does not specify a parent' % node_id)
        if node['parent'] == node_id:
            exit('error: node \'%s\' cannot have itself as a parent')
        if node['parent'] not in configs:
            exit('error: parent \'%s\' of node \'%s\' does not exist' % (node['parent'], node_id)

        # check module
        if 'module' not in node:
            exit('error: node \'%s\' does not specify a module' % node_id)
        if node['module'] not in supported_modules:
            exit('error: module \'%s\' for node \'%s\' does not exist' % (node['module'], node_id))

        # check module options
        if 'options' not in node:
            print('warning: node \'%s\' does not have options for module \'%s\'' % (node_id, node['module']))
            node['options'] = {}
        else:
            if node['module'] == 'deskew':
                node['options'] = sanitize_deskew_opts(node['options'])
            elif node['module'] == 'decon':
                node['options'] = sanitize_decon_opts(node['options'])
            elif node['module'] == 'mip':
                node['options'] = sanitize_mip_opts(node['options'])
            else:
                exit('error: no sanitization method for module \'%s\'' % node['module'])

        # check bsub options
        if 'bsub' in node:
            node['bsub'] = sanitize_bsub_opts(node['bsub'])
        else:
            node['bsub'] = {}
        
        return node

"""
Converts a config.json file into a pipeline graph
"""
def convert(configs):
    # check for required nodes
    if 'datasets' not in configs:
        exit('error: config.json requires a \'datasets\' node')
    elif 'root' not in configs['datasets']:
        exit('error: config.json requires a \'root\' field within the \'datasets\' node')

    # check that each node has a unique name
    node_ids = set()
    for node_id in configs:
        if node_id in node_ids:
            exit('error: node name \'%s\' is duplicated. Nodes must have unique names.' % node_id)
        else:
            node_ids.add(key)

    # check format of each node
    for node_id in configs:
        configs[node_id] = sanitize_node(configs, node_id)

    return pipeline

"""
Command-line
"""
if __name__ == '__main__':
    # get command line arguments
    args = parse_args()

    # read config to memory
    if args.input.is_file():
        with args.input.open(mode='r') as f:
            try:
                j = json.load(f)
            except json.JSONDecodeError as e:
                exit('error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
    else:
        exit('error: path \'%s\' is not a file' % args.input)

    # convert
    pipeline = convert(j)

    print(pipeline)