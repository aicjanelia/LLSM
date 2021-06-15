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

    """
    Prints class variables
    """
    def __repr__(self):
        s = '\nDatum - %s\n\tname = %s' % (self.path, self.name)

        if self.is_macro:
            s += '\n\titers = %s' % (self.iters)
        if self.is_dualcam:
            s += '\n\tcamera = %s' % (self.camera)

        s += '\n\tchannel = %i\n\tstack = %i\n\tlaser = %i nm\n\trel time = %i ms\n\tabs time = %i ms\n\tx = %i\n\ty = %i\n\tz = %i\n\tt = %i'\
                % (self.channel, self.stack, self.laser, self.time_rel, self.time_abs, self.x, self.y, self.z, self.t)
        
        return s

    """
    Prints class variables
    """
    def __str__(self):
        return self.__repr__()

    """
    Generates a datum from a filename
    """
    def generate(self):
        # parse filename
        j = filename2json.convert(self.path)

        # store json as class variables
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

    def tag_filename(self, string):
        return self.path.stem + string + self.path.suffix

class Dataset:
    def __init__(self, path):
        self.path = path
        self.is_skewed = None
        self.settings = None
        self.data = []
        self.generate()

    """
    Prints class variables
    """
    def __repr__(self):
        s = 'Dataset - %s' % (self.path)

        for datum in self.data:
            s += str(datum)
        
        return s

    """
    Prints class variables
    """
    def __str__(self):
        return self.__repr__()

    """
    Generates a dataset from a directory path
    """
    def generate(self):
        # get list of all files in path
        file_paths = [f for f in self.path.iterdir() if f.is_file()]

        # parse metadata
        for f in file_paths:
            # parse TIFF filename
            if str(f).lower().endswith(('.tif', '.tiff')):
                self.data.append(Datum(f))

            # parse Settings.txt
            if str(f).endswith('Settings.txt') and (self.settings == None):
                self.settings = settings2json.convert(f)

        # store settings json as class variables
        if 'waveform' not in self.settings:
            exit('error: settings file did not contain a Waveform section')
        if 'z-motion' not in self.settings['waveform']:
            exit('error: settings file did not contain a Z motion field')
        self.z_motion = self.settings['waveform']['z-motion']

        if self.z_motion == 'Sample piezo':
            if 's-piezo' not in self.settings['waveform']:
                exit('error: settings file did not contain a S Piezo field')
            if 'interval' not in self.settings['waveform']['s-piezo']:
                exit('error: settings file did not contain a S Piezo Interval field')
        self.steps = self.settings['waveform']['s-piezo']['interval']

        return 