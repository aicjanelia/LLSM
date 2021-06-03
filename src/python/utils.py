"""
utils.py
A collection of LLSM pipeline helper methods
"""

import json
from sys import exit

"""
Loads a json file into memory as a dictionary
"""
def load_json(path, fail=True):
    if path.is_file():
        with path.open(mode='r') as f:
            try:
                j = json.load(f)
            except json.JSONDecodeError as e:
                exit(f'error: \'%s\' is not formatted as a proper JSON file...\n%s' % (path, e))
        
        return j
    else:
        if fail:
            exit('error: path \'%s\' is not a file' % path)
        else:
            return {}