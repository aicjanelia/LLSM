---
title: Deconvolution
layout: default
nav_order: 6
---

# Deconvolution

The `decon` function performs a [Richardson-Lucy deconvolution](https://en.wikipedia.org/wiki/Richardson%E2%80%93Lucy_deconvolution) on the input image. The input image can be a deskewed image (in the case of stage scanning) or a raw input image (in the case of objective scanning). The function requires a centered PSF kernel, the PSF z-step, and the image z-step.

The PSF is input to the pipeline by including it in the paths portion of the configuration file. The directory containing PSF files is is specified, and then specific file names inside that folder are linked to the laser lines. Any filename is appropriate as long as it is specified, but the laser values themselves must exactly match the `Settings.txt file` (e.g., don't use 561 when it should be 560). The input PSF must be centered, and ideally cropped to a square. We provide an [ImageJ macro](https://github.com/aicjanelia/LLSM/blob/master/src/imagej/center-psf.ijm) that will take an image stack and perform this cropping for you. There must be a matching settings file for each PSF and it must have a matching name. For example, a PSF file named `cropped_560_PSF.tif` must have a corresponding file  `cropped_560_PSF_Settings.txt`.

If using the pipeline, the image and PSF z-steps will be automatically parsed. If you using this module directly, those values can be determined from the image and PSF settings files (see [metadata](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html#metadata)).

 Other inputs are `n`, the number of Richardson-Lucy iterations, and `subtract`, which subtracts a uniform value from the deconvolved data. For the AIC LLSM and MOSAIC, this subtract value should be 100 (based on the properties of our cameras).

# Usage

### Pipeline: Configuration File
This is an example portion of a configuration file to use deconvolution.

```json
"paths": {
    "root": "/path/to/experiment/",
    "psf": {
        "dir": "calibration",
        "laser": {
            "560": "cropped_560_PSF.tif",
            "488": "cropped_488_PSF.tif"
        }
    }
}
"decon": {
    "n": 5,
    "bit-depth": 16,
    "subtract": 100.0
}
```

### Command Line Example
When directly using the decon module on the command line, you must specify the input and output files as well as the PSF. You should also input the number of iterations. Pixel size, etc. should be specified to avoid any conflicts with default values. The example below will deconvolve an input image stack that was previously deskewed.
```c
decon -n 5 -b 16 -s 100.0 -w -k /path/to/calibration/cropped_488_PSF.tif -p 0.1 -q 0.21462536238843902 -o /path/to/experiment/decon/scan_Cam1_ch0_tile0_t0000_decon.tif /path/to/experiment/deskew/scan_Cam1_ch0_tile0_t0000_deskew.tif
```

### Decon Options

```text
decon: deconvolves an image with a PSF or PSF parameters
usage: decon [options] path

Allowed options:
  -h [ --help ]                       display this help message
  -k [ --kernel ] arg                 kernel file path
  -n [ --iterations ] arg             deconvolution iterations
  -x [ --xy-rez ] arg (=0.104000002)  x/y resolution (um/px)
  -p [ --kernel-spacing ] arg (=1)    z-step size of kernel
  -q [ --image-spacing ] arg (=1)     z-step size of input image
  -s [ --subtract-constant ] arg (=0) constant intensity value to subtract 
                                      from input image
  -o [ --output ] arg                 output file path
  -b [ --bit-depth ] arg (=16)        bit depth (8, 16, or 32) of output image
  -w [ --overwrite ]                  overwrite output if it exists
  -v [ --verbose ]                    display progress and debug information
  --version                           display the version number
```
