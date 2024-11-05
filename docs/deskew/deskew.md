---
title: Deskewing
layout: default
nav_order: 4
---

# Deskewing

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

The `deskew` function performs an affine transformation on a LLSM TIFF image to correct the shearing of the data during acquisition. The step/interval parameter refers to the distance travelled by the stage for each slice. This is not the same as the z-step which is equal to `step * sin(31.8) * (PI/180.0)`.