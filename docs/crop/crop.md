---
title: Cropping
layout: default
nav_order: 3
---

# Cropping

The `crop` module removes pixels from the sides of an image. In most cases, this module is not necessary. However, in some experimental set ups, it maybe useful to remove some pixels before processing, so save on both processing time and final image storage. As an example, there may be a need to leave a large margin around the sample during imaging if sample migration is expected. After the time lapse is completed, if the sample only moved to the left, the empty margins on the right could be removed.

When used though the pipeline command, the only parameters to be specified are the amount of pixels to crop from each side of the image. These should be the number of pixels to be removed, not the size of either the initial or final image. Thus, to remove the right 10 pixels from a 512 x 1500 x 201 pixel image stack, set `"cropLeft": 10`, which will lead to the output of a 502 x 1500 x 201 pixel image stack. `cropLeft` and `cropRight` remove pixels in the x-direction, `cropTop` and `cropBottom` remove pixels in the y-direction, and `cropFront` and `cropBack` remove pixels in z (i.e., remove z-slices). If an input stack is opened in ImageJ or FIJI, there will be a z-slider at the bottom of the image; `cropFront` removes slices starting from ImageJ's z = 1, while `cropBack` removes slices starting from the final z-slice. If any values are excluded, they are set to zero.

If directly using the crop module on the command line, these cropping values are provided as one list in the input variables. You must manually specify the paths to the input and output files, as well as metadata such as the z-step size.


# Usage

### Pipeline: Configuration File
The example below removes top 10 pixels in y, no and no pixels from the bottom in y. It removes 15 pixels from the left of the image in x. Because `cropRight` is excluded, no pixels are removed from the right. It also removes the first 100 z-slices and the last 50 z-slices.

```json
"crop": {
    "cropTop": 10,
    "cropBottom": 0,
    "cropLeft": 15,
    "cropFront": 100,
    "cropBack": 50
}
```

### Command Line Example
The following command removes 40 pixels from the top and no other cropping is performed. The input file is `/path/to/experiment/scan_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0004732481msecAbs_000x_000y_000z_0000t.tif` and the output (`-o`) will be saved to `/path/to/experiment/crop/scan_Cam1_ch0_tile0_t0000_crop.tif`. The input image has a z-step size of 0.25 (`-s 0.25`) and any existing output will be overwritten (`-w`)
```c
crop -c 40,0,0,0,0,0 -w -s 0.25 -o /path/to/experiment/crop/scan_Cam1_ch0_tile0_t0000_crop.tif  /path/to/experiment/scan_CamA_ch0_CAM1_stack0000_488nm_0000000msec_0004732481msecAbs_000x_000y_000z_0000t.tif
```

### Crop Options

```
crop: cropping boundary pixels
usage: crop [options] path

Allowed options:
  -h [ --help ]                    display this help message
  -c [ --crop ] arg (=0,0,0,0,0,0) boundary pixel numbers 
                                   (top,bottom,left,right,front,back)
  -o [ --output ] arg              output file path
  -x [ --xy-rez ] arg (=-1)        x/y resolution (um/px)
  -s [ --step ] arg (=-1)          step/interval (um)
  -b [ --bit-depth ] arg (=16)     bit depth (8, 16, or 32) of output image
  -w [ --overwrite ]               overwrite output if it exists
  -v [ --verbose ]                 display progress and debug information
  --version                        display the version number
```