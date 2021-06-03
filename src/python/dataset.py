from sys import exit

import settings2json

class Dataset:
    def __init__(self, path):
        self.path = path
        self.generate()

    def generate(self):
        


    def tag_filename(filename, string):
        f = PurePath(filename)
        return f.stem + string + f.suffix