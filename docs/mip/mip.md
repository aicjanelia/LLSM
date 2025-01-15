---
title: Maximum Intensity Projection
layout: default
nav_order: 7
---

# Maximum Intensity Projection (MIP)

The `mip` module is run together with the other processing modules and will create MIPs for key modules that are run. For example, if running both `deskew` and `decon`, MIPs will be made for both. The subfolder created by `mip` will contain subfolders indicating which input images were used to create them (e.g., a `decon` subfolder). Inside this folder will be files that end with `mip_x`, `mip_y`, and `mip_z`. The dimension in the file name is the axes over which the images have been projected.


| Modules Enabled | MIPs Created |
| ----- | ----- |
| mip only | input images |
| flatfield| flatfield |
| crop | crop |
| crop + flatfield | crop |
| (crop and/or flatfield or none) + deskew | deskew |
| decon + mip | input images & decon |
| flatfield + decon | flatfield & decon |
| crop +/- flatfield + decon | crop & decon |
| (crop and/or flatfield or none) + deskew + decon | deskew & decon |
| (crop and/or flatfield or none) + decon-first | deskew_after_decon |
| (crop and/or flatfield or none) + decon-first + deskew | deskew_after_decon + deskew |
| (crop and/or flatfield or none) + decon-first + deskew + decon | deskew_after_decon + deskew + decon |


## Viewing MIPs

2D MIPs are very useful for screening data, but in general, should not be used for downstream analysis (use the appropriate processed image folder of 3D images). To quickly look at MIPs in ImageJ, use the option under `File > Import > Image Sequence...`. This will open a dialog box that allow for selecting the directory of images to load in. The directory will be something like `/path/to/experiment/mip/deskew/`. By default, the importer will attempt to open all images in the folder. This begins by opening the first image in the folder, and then excluding any subsequent files that are not the same size. In the case of `mip` folders, this will lead to importing only `_x` files. To get only the files of interest, the `Filter:` dialog can be used to select files. For example, to select the z projections, input `_z` in the filter dialog.

More complex filtering of imported images is also possible using regular expressions. In ImageJ, `.*` is a wildcard, and the complete regular expression must be enclosed in parentheses. For example, to load in y projections of only channel zero, use `(ch0.*_y)` in the filter dialog. When the filter dialog box is changed, the number of images to be loaded automatically changes (shown in the `Count:` dialog). If the value is `---` or otherwise does not make sense, there is likely a typo in the regular expression.


# Usage

### Pipeline: Configuration File
In the configuration file, simply specify which axes to create (`true`) or ignore (`false`). In general, set all values to `true`.

```json
"mip": {
    "x": true,
    "y": true,
    "z": true
}
```

### Command Line Example
When directly using the mip module on the command line, you must specify the input and output files. You should also input the axes to project over (`-x -y -z`). The following example makes MIPs in all three dimensions for a deskewed image. Specify the pixel sizes to output properly scaled projections.
```c
mip -x  -y  -z  -p 0.104 -q 0.21462536238843902 -o /path/to/experiment/mip/deskew/scan_Cam1_ch0_tile0_t0000_deskew_mip.tif /path/to/experiment/deskew/scan_Cam1_ch0_tile0_t0000_deskew.tif
```

### MIP Options

```text
mip: generates a maximum intensity projection along the specified axes
usage: mip [options] path

Allowed options:
  -h [ --help ]                      display this help message
  -x [ --x-axis ]                    generate x-axis projection
  -y [ --y-axis ]                    generate y-axis projection
  -z [ --z-axis ]                    generate z-axis projection
  -p [ --xy-rez ] arg (=0.104000002) x/y resolution (um/px)
  -q [ --z-rez ] arg (=0.104000002)  z resolution (um/px)
  -o [ --output ] arg                output file path
  -b [ --bit-depth ] arg (=16)       bit depth (8, 16, or 32) of output image
  -w [ --overwrite ]                 overwrite output if it exists
  -v [ --verbose ]                   display progress and debug information
  --version                          display the version number
```

