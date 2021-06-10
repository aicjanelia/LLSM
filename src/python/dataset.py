import os
from sys import exit

import re

import settings2json
import filename2json

class Datum:
    def __init__(self, path):
        self.path = path
        self.name = None
        self.iters = None
        self.camera = None
        self.channel = None
        self.stack = None
        self.laser = None
        self.time_rel = None
        self.time_abs = None
        self.x = None
        self.y = None
        self.z = None
        self.t = None

        self.is_macro = None
        self.is_dualcam = None

        self.generate()

    def generate(self):
        # parse filename
        j = filename2json(self.path)

        # store json as object variables
        if 'name' not in j:
            exit('error: could not parse user-specified name field')
        self.name = j['name']

        if 'iters' in j:
            self.is_macro = True
            self.iters = j['iters']
        else:
            self.is_macro = False

        if 'camera' in j:
            self.is_dualcam = True
            self.camera = j['camera']
        else:
            self.is_dualcam = False

        if 'channel' not in j:
            exit('error: could not parse channel field')
        self.channel = j['channel']

        if 'stack' not in j:
            exit('error: could not parse stack field')
        self.stack = j['stack']

        if 'laser' not in j:
            exit('error: could not parse laser field')
        self.laser = j['laser']

        if 'time_rel' not in j:
            exit('error: could not parse relative time field')
        self.time_rel = j['time_rel']

        if 'time_abs' not in j:
            exit('error: could not parse absolute time field')
        self.time_abs = j['time_abs']

        if 'x' not in j:
            exit('error: could not parse x index field')
        self.x = j['x']

        if 'y' not in j:
            exit('error: could not parse y index field')
        self.y = j['y']

        if 'z' not in j:
            exit('error: could not parse z index field')
        self.z = j['z']

        if 't' not in j:
            exit('error: could not parse t index field')
        self.t = j['t']

class Dataset:
    def __init__(self, path):
        self.path = path
        self.settings = None
        self.data = []
        self.generate()

    def generate(self):
        # get list of all files in path
        file_paths = [f for f in self.path.iterdir() if f.is_file()]

        # parse metadata
        for f in files:
            # parse TIFF filename
            if str(f).lower().endswith(('.tif', '.tiff')):
                self.data.append(Datum(f))

            # parse Settings.txt
            if str(f).endswith('Settings.txt') and (self.settings == None):
                self.settings = settings2json.convert(f)

        return 

    def tag_filename(filename, string):
        f = PurePath(filename)
        return f.stem + string + f.suffix