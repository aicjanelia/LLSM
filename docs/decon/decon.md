---
title: Deconvolution
layout: default
nav_order: 5
---

# Deconvolution

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

The `decon` function performs a Richardson-Lucy deconvolution on a deskewed TIFF image. The function requires a centered PSF kernel, the PSF z-step, and the image z-step.