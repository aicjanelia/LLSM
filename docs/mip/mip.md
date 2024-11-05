---
title: Maximum Intensity Projection
layout: default
nav_order: 4
---

# Maximum Intensity Projection (MIP)

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

The `mip` function performs a maximum intensity projection on any 3D TIFF image. The function creates mips along any specified axis (i.e., `-x`, `-y`, or `-z`), and the output filenames are appended with the corresponding axis name. The `x` and `y` dimensions are scaled to match the resolution of the `z` dimension.