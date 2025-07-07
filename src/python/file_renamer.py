"""
Code to rename MOSAIC files in BigDataViewer scheme
in which the camera, channel, time and tile are all readable
by the automatic (and manual) importer(s).

The code takes four command line arguments:

    inpath -- path to the folder containing the data to rename

    fflag -- type of file to rename, e.g. mip, decon, deskew (raw if raw data)
    
    typeflag -- acquision type

            scan -- single-position acquisition
            
            tile -- multi-position acquisition (either tiled or disconnected)
            
    -- dry-run -- print names to output.txt instead of renaming (for testing)
    
A dry-run running of the code for single-position decon data would take the following form:

    python file_renamer.py ../temp/data/path decon scan --d       
    
to then actually rename the files, run the same command without the '--d' flag 

"""
import os
import sys
import re
import numpy as np
import argparse
from pathlib import Path, PurePath
import itertools

def parse_args():
    parser = argparse.ArgumentParser(description='Simple file renamer for putting MOSAIC images into BDV-compatible format')
    parser.add_argument('inpath', type=Path, help='path to the data')
    parser.add_argument('fflag', type=str, help='state of file to rename: e.g. mip, decon, deskew')
    parser.add_argument('typeflag', type=str, help='acquisition type: scan or tile')
    parser.add_argument('--dry-run', '-d',default=False, action='store_true', dest='dryrun',help='execute by printing names to output.txt rather than renaming')
    args = parser.parse_args()
    return args

### Naive function to detect string pairs
### Could be improved with regex but this works for now
def string_finder(old_str, old_phrases):
    vars = []
    for str in old_phrases:
        new_str = old_str.partition(str)[2]
        # print(new_str)
        new_str1 = new_str.partition('_')[0]
        # print(new_str1)
        vars.append(new_str1)
    return dict(zip(old_phrases,vars))

### Helper function
def tag_filename(filename, string):
    f = PurePath(filename)
    return f.stem + '_' + string + f.suffix

### Rename the files
def file_renamer(attributes,folder,tag,atype,dryrun=False):
    ch_A = []
    ch_B = []
    new_ch_B = []
    fnames = []
    tiles = []
    pattern_tile = re.compile(r'(\_(?:[-]*\d{1,}?){1}x\_(?:[-]*\d{1,}?){1}y\_(?:[-]*\d{1,}?){1}z_)')
    chN_A = 0
    
    # Search for number of CamA channels to reset CamB channel number
    for count, filename in enumerate(os.listdir(folder)):
        ext = Path(filename).suffix
        if ext=='.tif':
            m = re.findall(pattern_tile,filename)
            if m:
                if m[0] not in tiles:
                    tiles.append(m[0])
            temp = re.findall(r"CamA", filename)
            if temp:
                temp_atts = ['ch']
                details = string_finder(filename,temp_atts)
                if details[temp_atts[0]] not in ch_A:
                    ch_A.append(details[temp_atts[0]])
                    chN_A = np.amax([chN_A, int(details[temp_atts[0]])])
                    
    # Handle tiling
    tiles_dict = {tiles[i]:('_tile'+str(i)) for i in range(len(tiles))}
    chN_A += 1
    original_stdout = sys.stdout
    
    # Rename files (or print to output.txt if --dry-run)
    with open(Path(folder) / Path('output.txt'),'w') as file:
        sys.stdout = file
        for count, filename in enumerate(os.listdir(folder)):
            
            ext = Path(filename).suffix
            if ext=='.tif':
                details = string_finder(filename,attributes)
                sys.stdout = file
                temp = re.findall(r"CamA", filename)
                
                # Test if tile names are needed
                tile_temp = re.findall(pattern_tile, filename)
                if tile_temp[0] in tiles_dict:
                    tile = tiles_dict[tile_temp[0]]
                # Keep tile_0 if not
                else:
                    tile = '_tile0'
                    
                # New File names
                if bool(temp):
                    chStr = details[attributes[1]]
                    chStr = str(chStr).zfill(2)
                    # dst = f'scan_Cam_'+re.sub('A','0',details['Cam'])+'_ch_'+details['ch']+tile+'_t_'+details['stack']+'.tif'
                    dst = f'scan_ch'+chStr+tile+'_t'+details[attributes[-1]]+'.tif'
                else:
                    chStr = int(details[attributes[1]])+10
                    chStr = str(chStr).zfill(2)
                    # dst = f'scan_Cam_'+re.sub('B','1',details['Cam'])+'_ch_'+str(int(details['ch'])+N_ch_CamA)+tile+'_t_'+details['stack']+'.tif'
                    dst = f'scan_ch'+chStr+tile+'_t'+details[attributes[-1]]+'.tif' # CamB will be offset by 10 to avoid overlapping file names

                src = Path(folder) / Path(filename)
                dst_new = Path(folder) / tag_filename(dst,tag)
                # os.rename(src,dst)
                fnames.append(dst_new)
                if dryrun:
                    print(filename)
                    print(dst_new.name)
                else:
                    os.rename(src,dst_new)
                    
        # Regardles of --dry-run, print this info to output.txt for reference
        # This keeps track of the old and new names to cross-reference
        print('Old Channel Names:')
        print('CamA: ' + ', '.join(ch_A))
        print('CamB: ' + ', '.join(ch_B))
        print('New Channel Names:')
        print('CamB: ' + ', '.join(new_ch_B))
        print('Old Tile Names')
        print('Tiles: ' + ', '.join(tiles))
        print('New Tile Names')
        print(', '.join([tiles_dict[key] for key in tiles_dict]))

    sys.stdout = original_stdout

    return fnames


def main():
    args = parse_args()

    #### Choose which naming scheme the MOSAIC is using
    #### 'scan' (should) cover any single-position acquisition
    #### 'tile' (should) cover any multi-position acquisition such as tiled or multiple disconnected positions
    if args.typeflag =='scan':
        ids = [r'Cam',r'ch',r'stack']
    elif args.typeflag == 'tile':
        ids = [r'Cam',r'ch',r'Iter_']

    file_renamer(ids,folder=args.inpath,tag=args.fflag,atype=args.typeflag,dryrun=args.dryrun)


if __name__ == '__main__':

    main()