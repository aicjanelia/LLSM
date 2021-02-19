#! /misc/local/python-3.8.2/bin/python3
"""
autoDeconSend
This script will scrub a high level folder (usually AIC visitor last name)
for any un-processed folders. If any are found, it will send it to the
computer cluster with i = 10, -u, -D, -S flags. It will pull
the z stage step size from the settings file as well as how many
channels it needs to send.
"""

import os
import re
from pathlib import Path


lastName = "Palmer"
dateFolder = "20190923"
#pattern = '*Settings.txt'
dir_path = Path("//nrs/aic/instruments/llsm")
data_path = dir_path / lastName
psf_path = dir_path / lastName / dateFolder / "Calibration"
dir_loc = set()
summary = {}
pattern = '.*Settings.txt$'
folder_pattern = re.compile('CPPdecon', re.IGNORECASE)
calib = re.compile('Calibration', re.IGNORECASE)


#Walk through entire directory, find settings files to determine
#which folders have time lapse, then add the folder to dir_loc


for root,dirname,files in os.walk(data_path):
    for file in files:
        if re.search(pattern,file):
            if 'CPPdecon' not in dirname:
                if not re.search(calib,root):
                    dir_loc.add(os.path.join(root))
                    
# Collecting Z step, wavelengths, and directory locations for files that need processing
dir_list = list(dir_loc)
dir_list = sorted(dir_list)
trunk = '~/cpudecon_qsub.sh'

for i in range(0,len(dir_list)):
    settings = [f for f in os.listdir(dir_list[i]) if re.search(pattern,f)]
      
    for j in range(0,len(settings)):
        fn = dir_list[i] + '//' + settings[j]
        text = Path(fn)
        f_contents = open(text,'r').read()  
        
#Determine if the data needs deskewed, then parse the stage step size        
        stage = f_contents.find('S Piezo')
        if stage != -1:
            substr = f_contents.find('S Piezo')
            idx = f_contents.find('0.',substr)
            z_step = f_contents[idx:idx+3]
            flags = '-z %s -D -S -u -b 100 -i %d -Z 0.1 -M 0 0 1' %(z_step, 2)
        else:
            substr = f_contents.find('Z PZT')
            idx = f_contents.find('0.',substr)
            z_step = f_contents[idx:idx+3]
            flags = '-z %s -u -b 100 -Z 0.1 -i %d -M 0 0 1' %(z_step, 5)
            
#Parse what lasers are used in what channels
        idx_waveform = f_contents.find('Waveform')
        idx_cam = f_contents.find('Camera')
        cnt_lsr = f_contents.count('Laser',idx_waveform, idx_cam)
        
        for k in range(0,cnt_lsr):
            parts = f_contents.split('Laser')
            pos = list()
            pos.append(parts[k+1][31:31+3])
            PSF_ch = pos[0] + '_PSF.tif'
            psf = str(psf_path / PSF_ch)

                
            output = trunk + ' ' + str(dir_list[i]) + ' ch' + str(k) + ' ' + psf + ' ' + flags
            os.system(output)
            print(output)


        
        
        
