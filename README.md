# Lattice Lightsheet Microscope (LLSM) Pipeline

## File Names and Organization

The LLSM produces a predictable, but not always informative, set of file names and configuration files. Below is a brief description of output produced by the AIC's LLSM (control software v 4.04505.Development).

**Every image acquisition generates a series of TIFF images and one or more settings files.**

### Image Files

The LLSM control software will not automatically organize image files into directories. Therefore, the user must specify where the images will be saved. At the AIC, we typically have users  

### Settings File(s)

Settings files are plain text documents containing parameters that describe the microscope's state and acquisition setup.

#### Normal Acquisition

In general, a single acquisition will generate a single settings file. If the user-defined name for the acquisition is `cell1`, then the settings file will be `cell1_Settings.txt`.

#### Looped Acquisition

The control software has a scripting mode that allows the user to define an acquisition mode that loops over different positions, time delays, etc. If loops are used in the scripting mode, then the control software will produce a new settings file for each iteration of the loop. If the user-defined name for the acquisition is `cell1`, then the settings files will be `cell1_Iter_0000_Settings.txt`, `cell1_Iter_0001_Settings.txt`, `cell1_Iter_0002_Settings.txt`, and so on.

Nested loops are also possible within the scripting mode. An `Iter_####` counter is added to the file name for each nested loop. For example, a script with one nested loop will have a name similar to `cell1_Iter_0000_Iter_0000_Settings.txt`.


A directory is created sometimes with the format:
TimeLapse\d
• For each stack there is a settings text document.
– This document starts with a user settable string (\w+ ). – The file name ends with (Settings.txt)
An example of two file names:
642 50mW 488 30mW Iter 000 Settings.txt cell04 Settings.txt
• Data of interest within this file:
–
–
–
• A 3D

The start date time group (Date : \d+ /\d+ /\d+ \d+ :\d+ :\d+ \[’AM’,’PM’] )
The z interval is in the second token (S PZT Offset, Interval (um), # of Pixels for Excitation (\d) :
\d+ \d+.\d+ \d+)
For each channel there will be an entry for the laser where the first token is the channel number and the second token
is the excitation wavelength (Excitation Filter, Laser, Power (%), Exp(ms) (\d ) : tif image is created for each channel and frame. The file name contains the following in order:
User settable ( \w+ ).
Optional iteration number (Iter \d+ ).
Camera number which is only present when there are multiple cameras (Cam\D ) The channel number (ch\d ).
The camera number unused, always (CAM1 ).
This is the frame number (stack\d+ ). Thewavelengthoftheimage’schannel(\d+nm). Deltatimefromstartofstackacquisition(\d+msec).
A CPU relative time field (\d+ msec). NOTE: there is no underscore after this field. Positiondatawhendoing3Dtiling(Abs\d+x\d+y\d+z).
There is a second frame field at the end (\d+ t).
Extension (.tif).
N/A \d+ \d+ \d+ )
An example of two different file names:
642 50mW 488 30mW Iter 000 CamA ch0 CAM1 stack0000 642nm 0000000msec 0009004434msecAbs 000x 000y 000z 0000t.tif cell04 ch0 CAM1 stack0000 488nm 0000000msec 0000266777msecAbs 000x 000y 000z 0000t.tif

## Toolbox

### Deskew

This code is responsible for deskewing volumetric images that were acquired using stage scanning (as opposed to objective scanning).

### Resave

Resaves TIFF images as Imaris IMS files.

### MIP

Generates maximum intensity projection videos.