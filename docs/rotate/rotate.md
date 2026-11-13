---
title: Rotation
layout: default
nav_order: 8
---

# Rotation

The `rotate` module rotates an image stack about the y axis to move it from the objective frame into the **coverslip frame**. After [deskewing](https://aicjanelia.github.io/LLSM/deskew/deskew.html), x and y lie in the objective plane and z is the objective normal; rotating by the objective angle makes z the coverslip normal. It is typically run after deskewing (and after deconvolution, if requested):

| Workflow | Order |
| ----- | ----- |
| (a) | deskew &rarr; rotate |
| (b) | deskew &rarr; decon &rarr; rotate |
| (c) | decon &rarr; deskew (`decon-first`) &rarr; rotate |

`rotate` can also be run alone, for example on objective-scanned data or on any stack you want to rotate by a custom angle.

# Which images are rotated

Each rotated stage is written to its own folder, named `rotate_` plus the name of the folder it was made from, so it is clear how the pipeline was run:

| Rotated stage (`stages` name) | Source folder | Output folder |
| ----- | ----- | ----- |
| `deskew` | `deskew/` | `rotate_deskew/` |
| `decon` (deconvolved after deskewing) | `decon/` | `rotate_decon/` |
| `deskew_after_decon` (`decon-first`) | `deskew_after_decon/` | `rotate_deskew_after_decon/` |
| none enabled | unprocessed (or cropped/flatfielded) images | `rotate/` |

By default the last stage of each chain is rotated: `decon` if deconvolution is enabled after deskewing, otherwise `deskew`, plus `deskew_after_decon` if `decon-first` is enabled. A configuration with both `deskew` and `decon-first` therefore rotates both chains separately. To rotate more than one stage of a chain (for example both the deskewed and the deconvolved images), list them with `stages`: `"stages": ["deskew", "decon"]`. When `stages` is given it replaces the default, so list every stage you want rotated. A stage can only be listed if the module that makes it is enabled.

# Angle and sign convention

`rotate -a θ` rotates the image **content** with the matrix

```
[  cos θ   0   sin θ ]
[    0     1     0   ]
[ -sin θ   0   cos θ ]
```

applied to physical (calibrated) coordinates, so x&prime; = x cos θ + z sin θ and z&prime; = -x sin θ + z cos θ. The rotation is done in micrometers, so the anisotropy of the data (xy pixels smaller than the z step) is accounted for. The y axis is the rotation axis and is not changed.

| Microscope | Rotation angle | Deskew angle in the pipeline |
| ----- | ----- | ----- |
| MOSAIC | -32.45 | 147.55 |
| LLSM | 31.8 | 31.8 |

Note that on the MOSAIC the rotation angle is -32.45, not the 147.55 used for deskewing; the two differ by 180&#176;. Any other angle can be used if there is an experimental reason to rotate by something else.

# Output size and spacing

The rotated volume is resampled once with linear interpolation. By default the output is **isotropic**: cubic voxels the size of the xy pixel (for example 0.108 x 0.108 x 0.108 &#956;m). Because the z spacing of deskewed data is coarser, the decrease in file size from cropping away empty regions of the image is offset by the increase in file size from changing the voxel sizes. Use `--preserve-spacing` (`"isotropic": false` in the configuration file) to keep the voxel size of the input (for example 0.108 x 0.108 x 0.268 &#956;m) instead; this decreases files sizes at the expense of resolution. Output voxel centers sit on a grid of integer multiples of the output spacing, relative to input voxel (0,0,0) and the absolute position of the output is not preserved.

Two cropping modes are available with `--crop-mode` (`"crop-mode"` in the configuration file):

| Mode | Output region |
| ----- | ----- |
| `keep-all` (default) | The bounding box of the valid data. The empty triangles created by deskewing are not included, but black space around the rotated data is. |
| `strict` | The largest rectangle that contains only valid data (no empty space). Only available when the rotation angle matches the deskew geometry (e.g. MOSAIC -32.45 after deskewing at 147.55). |

Both modes need to know where the valid data are. The pipeline passes the deskew shift (`--shift`, in pixels per z plane, `step*cos(deskew angle)/xy-res`, negative on the MOSAIC) to `rotate` automatically. When running `rotate` by hand on deskewed data, supply it yourself; without `--shift` the whole array is treated as valid data and `strict` is not available. On deskewed input with shift unspecified or set to zero, the empty deskew triangles are then rotated and included in the `keep-all` bounding box, which shows up as many extra, empty z slices, greatly increasing file size. `rotate` prints a warning when `--shift` is not given; pass `--shift 0` explicitly if the input really is not deskewed.

Prevoius work used Fiji's BigStitcher plugin for rotation. Small differences from BigStitcher's output (intensities within about +/-1 count, most likely from rounding when converting to the output bit depth, and a thin rim at the boundary of the data) are expected.

# Memory

Images are processed as double-precision numbers in memory, so a 2075 x 1500 x 401 stack needs about 10 GB for the input, plus the output (about 2 GB for `strict`, more for `keep-all`). Request enough slots in the `bsub` section of the configuration file (15 GB per slot); the deconvolution memory estimate in [Configuration File](https://aicjanelia.github.io/LLSM/pipeline/config.html#slots) does not apply to `rotate`.

# Usage

### Pipeline: Configuration File
This is an example portion of a configuration file for a MOSAIC acquisition that is deskewed and then rotated.

```json
"deskew": {
    "xy-res": 0.108,
    "fill": 0.0,
    "bit-depth": 16,
    "angle": 147.55
},
"rotate": {
    "angle": -32.45,
    "stages": ["deskew"],
    "crop-mode": "strict",
    "isotropic": true,
    "fill": 0.0,
    "bit-depth": 16
}
```

The xy pixel size and step size come from the `deskew` (or `decon-first`) section and the settings file. When `rotate` is used on its own, `xy-res` can be given in the `rotate` section; the z step is read from the settings file and only objective-scanned data are supported. Rotated images are saved in the folders described above, with MIPs in `mip/<folder>` if requested. `stages` is optional.

### Command Line Example
The command below rotates a deskewed MOSAIC image into the coverslip frame, keeping only valid data. The z spacing is `step*sin(angle)` (0.2683 &#956;m for a 0.5 &#956;m step) and the shift is `0.5*cos(147.55&#176;)/0.108 = -3.9068` pixels.
```
rotate -a -32.45 -x 0.108 -z 0.2683 --shift -3.9068 -c strict -w -o scan_ch00_tile0_t0000_rotate.tif scan_ch00_tile0_t0000_deskew.tif
```

### Crop Options
```
rotate: rotates an image stack about the y axis (e.g., from the objective to the coverslip frame)
usage: rotate [options] path

Allowed options:
  -h [ --help ]                      display this help message
  -a [ --angle ] arg                 rotation angle about the y axis (degrees),
                                     BigStitcher y-axis convention (MOSAIC:
                                     -32.45)
  -x [ --xy-res ] arg (=-1)          x/y resolution of the input (um/px);
                                     default is read from the input file
  -z [ --z-res ] arg (=-1)           z resolution of the input (um/px), i.e.
                                     step*sin(angle) after deskewing; default
                                     is read from the input file
  -d [ --shift ] arg (=0)            signed deskew shift (px per z plane) =
                                     step*cos(deskew angle)/xy-res; needed for
                                     strict cropping, 0 treats the whole array
                                     as valid
  -c [ --crop-mode ] arg (=keep-all) keep-all (bounding box of the valid data)
                                     or strict (largest fully valid region;
                                     needs --shift and a matching angle)
  -i [ --isotropic ]                 write isotropic output voxels (xy size);
                                     this is the default
  -p [ --preserve-spacing ]          preserve the input voxel spacing instead
                                     of writing isotropic voxels (smaller
                                     output)
  -f [ --fill ] arg (=0)             value used to fill empty regions
  -o [ --output ] arg                output file path
  -b [ --bit-depth ] arg (=16)       bit depth (8, 16, or 32) of output image
  -t [ --thread ] arg (=1)           number of threads
  -w [ --overwrite ]                 overwrite output if it exists
  -v [ --verbose ]                   display progress and debug information
  --version                          display the version number
```
