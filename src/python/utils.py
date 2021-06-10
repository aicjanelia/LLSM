"""
utils.py
A collection of LLSM pipeline helper methods
"""

import re
import json
from sys import exit


"""
Searches a string for a pattern and adds the first match to a given dictionary
"""
def find_first_pattern(data, key, string, pattern, cast_as=str):
    m = re.search(pattern, string, re.MULTILINE)

    if m:
        if cast_as == str:
            data[key] = m.group(1).strip()
        else:
            data[key] = cast_as(m.group(1).strip())

"""
Searches a string for a pattern and adds the last match to a given dictionary
"""
def find_last_pattern(data, key, string, pattern, cast_as=str):
    m = re.findall(pattern, string)

    if m:
        if cast_as == str:
            data[key] = m[-1].strip()
        else:
            data[key] = cast_as(m[-1].strip())

"""
Searches a string for a pattern and adds all matches to a given dictionary as a list
"""
def find_every_pattern(data, key, string, pattern, cast_as=str):
    m = re.findall(pattern, string)

    if m:
        if cast_as == str:
            data[key] = [s.strip() for s in m]
        else:
            data[key] = [cast_as(s.strip()) for s in m]

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