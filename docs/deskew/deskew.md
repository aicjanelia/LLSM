---
title: Deskewing
layout: default
nav_order: 5
---

# Deskewing

The `deskew` function performs an affine transformation on an image stack to correct the shearing of the data during acquisition. This step is only necessary if images were acquired in stage scanning mode (see [metadata](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html#metadata) to identify an experiment's scanning mode).

Images on the LLSM and MOSAIC are acquired at an angle with respect to the coverslip because of the orientation of the objectives.  To learn more about this, check out the AIC's blog post on [Understanding Objective Orientation in the LLSM](https://www.aicjanelia.org/post/understanding-objective-orientation-in-the-llsm).  The deskew module takes as an input the objective angle and the stage step size. Note that the stage step size (the amount that the stage moves between slices) is not the same as the z-step size in the final stack. Step sizes are automatically calculated by the pipeline's setting parsers, and can also be determined manually from the `Settings.txt` file (see [metadata](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html#metadata)). You may also optionally specify the fill for empty regions in the skewed regions (default 0) and the bit-depth (default 16).

If you need to calculate the z-step (for example, for downstream processing of the images), use the following equation:
```
z-step = stage-step*sin(system-angle)
```
Make sure that the calculator that you use to calculate sin knows that the angle is in degrees, or convert the angle to radians (`radians = degrees*pi/180`). The relevant values for each microscope are listed in the table below.

| Microscope | Angle | &#956;m/pixel |
| ----- | ----- | -----|
| LLSM | 31.8&#176; | 0.104 |
| MOSAIC | -32.45&#176; = 147.55&#176; | 0.108 |

# Usage

### Pipeline: Configuration File
This is an example portion of a configuration file for a MOSAIC acquisition to be deskewed.

```json
"deskew": {
    "xy-res": 0.108,
    "fill": 0.0,
    "bit-depth": 16,
    "angle": 147.55
}
```

### Command Line Example
When directly using the deskew module on the command line, you must specify the input and output files. The best practice would be to also explicitly input values such as the angle, pixel size, etc. to avoid any conflicts with default values. The example below will deskew an input MOSAIC image stack.
```c
deskew -a 147.55 -x 0.108 -f 0.0 -b 16 -w -s 0.4 -o /path/to/experiment/deskew/scan_Cam1_ch0_tile0_t0000_deskew.tif  /path/to/experiment/scan_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0004732481msecAbs_000x_000y_000z_0000t.tif
```

### Deskew Options

```text
deskew: removes skewing induced by sample scanning
usage: deskew [options] path

Allowed options:
  -h [ --help ]                      display this help message
  -x [ --xy-rez ] arg (=0.104000002) x/y resolution (um/px)
  -s [ --step ] arg                  step/interval (um)
  -a [ --angle ] arg (=31.7999992)   objective angle from stage normal 
                                     (degrees)
  -f [ --fill ] arg (=0)             value used to fill empty deskew regions
  -o [ --output ] arg                output file path
  -b [ --bit-depth ] arg (=16)       bit depth (8, 16, or 32) of output image
  -w [ --overwrite ]                 overwrite output if it exists
  -v [ --verbose ]                   display progress and debug information
  --version                          display the version number
```
