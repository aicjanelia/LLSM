---
title: File Organization
layout: default
parent: Pipeline Usage
nav_order: 3
---

# File Organization
{: .no_toc }

## Table of Contents
{: .no_toc .text-delta }

1. TOC
{:toc}

Users are encouraged to organize experiments by date and with informative folder structures before running the processing pipeline. All processed files are saved in subfolders of the original image folder. Processed files can be saved with the [original naming scheme](#raw-images) or a simplified [bdv naming scheme](#bdv-saving).

A common mistake is to start an acquisition, immediately realize a mistake, end the first acquisition, and restart it in the same folder. This can lead to incorrect processing of the folder as it will have multiple files with the same time point naming, etc. If restarting an acquisition, always create a new folder!

##  Raw Images
After a LLSM or MOSAIC acquisition, the acquisition folder will contain individual tiff files for each time point and channel. The same folder that contains tiff files will also contain several metadata files.  The structure of the metadata files will depend on the acquisition set up, but most importantly for the pipeline, the folder should contain at least one file ending with `Settings.txt`.

Each raw tiff file name also contains relevant metadata for the experiment. Two example filenames from the LLSM are:
```
scan_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0004732481msecAbs_000x_000y_000z_0000t.tif
scan_CamA_ch0_CAM1_stack0001_488nm_0002520msec_0004735001msecAbs_000x_000y_000z_0000t.tif
```
In this example, the acquisition was named `scan`. This is the most common naming, but it is possible to have other names here. If changing the name from scan, make sure to not include any spaces in your filenames.

The images were acquired on Camera A (`CamA`); the only other option is `CamB`.  This file corresponds to channel 0 (`ch0`), which we can see later in the filename corresponds to the `488nm` laser.

The first filename corresponds to the first time point, as indicated by `stack0000`, the second filename is the next time point as indicated by `stack0001`, and this pattern will continue across the time course. Relative (e.g., `0000000msec`) and absolute (e.g., `0004732481msecAbs`) times of acquisition are also recorded in the file name. These can be used to confirm the frame rate; in this example there were 2.52 s between stack 0 and stack 1.

### _Multi-position Imaging_
If multiple positions are acquired at once (whether for tiling or to acquire distinct regions of the coverslip), the file names will change slightly.  Here are example file names from a multi-position imaging experiment on the MOSAIC:
```
Scan_Iter_0000_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0005394318msecAbs_-01x_-01y_-01z_0000t
Scan_Iter_0001_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0005695970msecAbs_-01x_-01y_-01z_0001t
```
In this case, the frame number is no longer recorded in the stack position of the file name. You can see in the example above that both names contain `stack0000`. The frame number now shows up in two other places in the filename: `Iter_####` and `####t`.

In a multi-position experiment, the separate positions will be stored in their own individual sub-folders. In the case of a tiling experiment, the tile number and position will be encoded in the `*x_*y_*z_` portion of the file name.

### _Metadata_
The `Settings.txt` file is parsed by the subfunctions `settings2json` for the LLSM or `mosaicsettings2json` for the MOSAIC. It parses multiple parts of the settings file, but two key parts for processing are the z-stack and laser settings, which are both contained in the `***** Waveform *****` section of the file.

Both systems can either be run in stage scanning or objective scanning mode. Stage scanning mode is most commonly used and requires deskewing. In objective scanning mode, no deskewing is required. To confirm which mode on the lattice, look for a section on `Z motion`, while on the MOSAIC, look for the name of the stage offset section. The text to search for in the settings file is summarized below.

| System | Scan Type | Identifying Settings Text |
| ------ | ------ | ------ |
| LLSM | Objective Scanning | Z galvo & piezo |
| LLSM | Stage Scanning | Sample piezo |
| MOSAIC | Objective Scanning | XZ stage Offset |
| MOSAIC | Stage Scanning | X Stage Offset |

This same section also contains information on the z-step interval. For objective scanning, the z-step is the second numerical value on the line that starts with `S Piezo Offset` (LLSM) or `XZ stage Offset` (MOSAIC).  For stage scanning, look for the second number after `S Piezo Offset` (LLSM) or `X stage Offset` (MOSAIC), which is the step of the stage, but not the final z-step size. To determine the z-step size for stage scanning, use the equation `z-step = stage-step*sin(system-angle)`. The system-angle is 31.45&#176; on the LLSM and -32.45&#176; on the MOSAIC.

```c
// Example MOSAIC Settings File
X Stage Offset, Interval (um), # of Pixels for Excitation (0) :	5	0.4	501

// Calculations
X Stage Offset = Stage Scanning
Stage step = 0.4
Z-step = 0.4*sin(32.45 degrees) = 0.21462536238
```

Laser information is encoded on sequential lines as seen in the examples from each system below. The order of the lasers in this section will be used to identify channels with the appropriate PSFs for deconvolution.
```c
// LLSM Example
Excitation Filter, Laser, Power (%), Exp(ms) (0) :	N/A	560	10	10
Excitation Filter, Laser, Power (%), Exp(ms) (1) :	N/A	647	50	10
Excitation Filter, Laser, Power (%), Exp(ms) (2) :	N/A	488	10	10

// MOSAIC Example
Excitation Filter, Laser, Power (%), Exp(ms), Laser2, Power2 (%), Laser3, Power3 (%) (0) :	N/A	560	10	20	OFF	0.1	OFF	0.1
Excitation Filter, Laser, Power (%), Exp(ms), Laser2, Power2 (%), Laser3, Power3 (%) (1) :	N/A	642	10	20	OFF	0.1	OFF	0.1
```


## Processed Files
Each module that is run will create its own subfolder of files. Subsequent modules will look in the relevant subfolder to continue the processing. For example, if running both cropping and deskewing, the deskewing will be run on the files in the `crop` folder, not on the original raw images. Modules are run in the order `flatfield > crop > deskew > decon`. If the configuration file contains a section for `decon-first`, the order of deconvolution and deskewing will be swapped. See [Deconvolution](https://aicjanelia.github.io/LLSM/decon/decon.html) for more about `decon-first`, which saves files in folders called `decon_before_deskew` and `deskew_after_decon`.

### _Maximum Intensity Projections (MIPs)_
Maximum Intensity Projections take the maximum intensity across one axis of an image to turn a 3D image into a 2D image. This can be useful for quickly reviewing images without needing to load the full experiment into memory. See more about this in the [MIP documentation](https://aicjanelia.github.io/LLSM/mip/mip.html). In general, analysis should be done on the 3D images, and thus the `mip` folder should not be used for analysis.

MIPs are created for key modules that are used, so inside the `mip` folder will be additional subfolders corresponding to modules (e.g., `deskew` and `decon`). The folder name indicates which input image was used to create the MIP. MIPs can be created by taking the maximum along the x, y, or z axes, and file names will reflect this by ending with `_x`, `_y`, and `_z` respectively.

## BDV Saving
The raw image file names contain informative metadata, but the long complex names can make converting to other file formats (e.g., Imaris) difficult. When bdv saving is enabled in the [configuration file](https://aicjanelia.github.io/LLSM/pipeline/config.html), the raw images will keep their informative names, but all processed files will have simplified names that are inspired by the [Automatic Loader in BigSticher](https://imagej.net/plugins/bigstitcher/autoloader), which runs on BigDataViewer (bdv). The simplified version keeps track of channels, tiles, and time points as simple numeric values (e.g., the specific laser line is removed). An example of the naming convention is shown below for a raw image that was run through the crop module.

In a previous version of this naming scheme, the camera information was also recorded in the simplified file naming, but that has been removed in the current version.  When using the MOSAIC pipeline, all channels from Camera B will have 10 added to their channel value (so ch0 becomes ch10, ch1 becomes ch11, etc.). This keeps simultaneous acquisitions on Camera A and B from re-using the same channel number when naming files.

```c
// raw image
scan_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0107885054msecAbs_-01x_-01y_-01z_0000t.tif
// cropped image
scan_ch0_tile0_t0000_crop.tif
```

Enable bdv saving by including the following in the configuration file:
```json
"bdv": {
    "bdv_save": true
}
```