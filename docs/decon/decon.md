---
title: Deconvolution
layout: default
nav_order: 6
---

# Deconvolution
{: .no_toc }

The `decon` function performs a [Richardson-Lucy deconvolution](https://en.wikipedia.org/wiki/Richardson%E2%80%93Lucy_deconvolution) on the input image. The input image can be a deskewed image (in the case of stage scanning) or a raw input image (in the case of objective scanning). The function requires a centered PSF kernel, the PSF z-step, and the image z-step.

## Table of Contents
{: .no_toc .text-delta }

1. TOC
{:toc}

## Deconvolution Inputs
The PSF is input to the pipeline by including it in the paths portion of the configuration file. The directory containing PSF files is is specified, and then specific file names inside that folder are linked to the laser lines. Any filename is appropriate as long as it is specified, but the laser values themselves must exactly match the `Settings.txt file` (e.g., don't use 561 when it should be 560). The input PSF must be centered, and ideally cropped to a square. There must be a matching settings file for each PSF and it must have a matching name. For example, a PSF file named `cropped_560_PSF.tif` must have a corresponding file  `cropped_560_PSF_Settings.txt`. To prepare a bead image to be used as PSF in the deconvolution, see [Point Spread Function for Deconvolution](https://aicjanelia.github.io/LLSM/decon/psf.html).

If using the pipeline, the image and PSF z-steps will be automatically parsed. If you using this module directly, those values can be determined from the image and PSF settings files (see [metadata](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html#metadata)).

 Other inputs are `n`, the number of Richardson-Lucy iterations, and `subtract`, which subtracts a uniform value from the deconvolved data. For the AIC LLSM and MOSAIC, this subtract value should be 100 (based on the properties of our cameras).

 ## Deconvolution Ordering
 In the original pipeline ordering, deskewing, if required, always came before deconvolution. Configuration files containing a section for `deskew` and `decon` will follow the rules of this initial ordering (see table). If the configuration file contains a section for `decon-first`, that section will result in a deconvolution followed by deskewing. If all three sections are included, both ways of processing will be peformed, which can be used for troubleshooting. Note that there is no reason to use `decon-first` for objective scanned data as it will result in unncessary deskewed files.

### _decon-first_
 When running decon-first, the PSF must be resampled before running the pipeline, as described in [Point Spread Function for Deconvolution](https://aicjanelia.github.io/LLSM/decon/psf.html). This resampled PSF must be specified in a separate section of the configuration file paths, as illustrated in [Pipeline: Configuration File](#pipeline-configuration-file). The pipeline then takes the input resampled PSF and skews it using the stage step of the experimental data and an angle of 180 minus the microscope objective angle. This results in a PSF that is skewed the same amount as the experimental data but in the opposite direction. The pipeline submits deskew jobs to the cluster to create one skewed PSF file per experimental directory and laser line. This file is saved in the experimental directory (not in the calibration folder). The pipeline checks if the files have been created, and waits 30 seconds at a time for the files to appear. If the files do not appear after three checks, the pipeline will create an error and will not submit and deconvolution jobs.  If the skewed PSF file has already been created, it will be used rather than overwritten.

 Once the skewed PSF files are created, the deconvolution module is run on the input files (or flatfield or crop files, as appropriate to the configuration file). The decon-first deconvolution command **assumes that the input images and the PSF have the same Z spacing**, regardless of what is recorded in the PSF settings file. To distinguish from the normal pipeline ordering, these files will be saved in a folder called *decon_before_deskew*.  After deconvolution, the deconvolved files are then deskewed and saved in *deskew_after_decon*. If MIPs are requested, they will be made for the *deskew_after_decon* files.

### _Pipeline Ordering_
| Modules in Configuration File | Decon Input File | Cluster Job Processing Steps |
| ------ | ------ | ------ |
| decon | input images | input > decon |
| flatfield + decon | flatfield | input > flatfield > decon |
| crop + decon | crop | input > crop > decon |
| flatfield + crop + decon | crop | input > flatfield > crop > decon |
| (crop and/or flatfield or none) + deskew + decon | deskew | input (> flatfield > crop) > deskew > decon |
| decon-first | input images | input > decon > deskew |
| flatfield + decon-first | flatfield | input > flatfield > decon > deskew |
| crop + decon-first | crop | input > crop > decon > deskew |
| flatfield + crop + decon-first | crop | input > flatfield > crop > decon > desekw |
| (crop and/or flatfield or none) + decon-first + deskew | input (or flatfield or crop) | input (> flatfield > crop) > decon > deskew followed by (input, flatfield, or crop) > deskew |
| (crop and/or flatfield or none) + decon-first + deskew + decon | input (or flatfield or crop) followed by deskew | input (> flatfield > crop) > decon > deskew followed by (input, flatfield, or crop) > deskew > decon |


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
    "subtract": 100.0,
    "xy-res": 0.104
}
```

This is an example portion of a configuration file to use decon-first, which first deconvolves and then deskews the data.

```json
"paths": {
    "root": "/path/to/experiment/",
    "psf": {
        "dir": "calibration",
        "resampled": {
            "560": "cropped_resampled_560_PSF.tif",
            "488": "cropped_resampled_488_PSF.tif"
        }
    }
}
"decon-first": {
    "decon": {
        "n": 5,
        "bit-depth": 16,
        "subtract": 100.0,
        "xy-res": 0.104
    },
    "deskew": {
        "angle": 31.8,
        "xy-res": 0.104,
        "fill": 0.0,
        "bit-depth": 16
    }
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
