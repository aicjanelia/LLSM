---
title: Home
layout: home
nav_order: 1
---
# LLSM Processing Pipeline

The Advanced Imaging Center (AIC) uses this pipeline for pre-processing of images created on our LLSM or MOSAIC systems. The pipeline consists of multiple discrete modules that are accessed via the command line using either the `llsm-pipeline` or `mosaic-pipeline` commands. Which modules to implement are controlled by a configuration JSON file. The main difference in the two pipelines is the parsing of filenames and acquisition settings metadata.

Each of the modules compiles to a separate binary that can also be directly executed by calling it on the command line. These modules are:
- Cropping
- Deskewing
- Deconvolution
- Maximum Intensity Projection

The main pipeline command and each individual module are further described in this documentation. With any command, you can also use the `-h` option to get a list of supported arguments.


## Cheatsheet

### Command Line Examples
```
llsm-pipeline /aic/instruments/llsm/experimentFolder/config.json

mosaic-pipeline /aic/instruments/MOSAIC/experimentFolder/config.json
```

### Configuration File
See an [example configuration file here](https://aicjanelia.github.io/LLSM/pipeline/config.html#example-configjson).

### Microscope Parameters
| Microscope | Angle | &#956;m/pixel |
| ----- | ----- | -----|
| LLSM | 31.8&#176; | 0.104 |
| MOSAIC | -32.45&#176; = 147.55&#176; | 0.108 |

