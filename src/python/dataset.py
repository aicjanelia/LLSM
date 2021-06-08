import os
from sys import exit

import re

import settings2json
import filename2json

class Datum:
    def __init__(self, path):
        self.path = path
        self.channel = None
        self.camera = None
        self.generate()

    def generate(self):
        # parse filename
        j = filename2json(self.path)
        self.channel = j['channel']
        self.camera = j['camera']
        

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