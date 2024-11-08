[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.11106928.svg)](https://doi.org/10.5281/zenodo.11106928) [![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# Lattice Light-Sheet Microscope (LLSM) Pipeline

This repository contains pipeline code for handling AIC's LLSM and MOSAIC data. The pipeline consists of multiple discrete modules: cropping, deskewing, deconvolution, and maximum intensity projection. The pipeline is accessed via the command line using the `llsm-pipeline` and  `mosaic-pipeline` commands.

For most cases, the `llsm-pipeline` and `mosaic-pipeline` commands should be all that is needed. However, the individual modules, which each compile to a separate binary, can be run directly on a single file if needed. Use the `-h` option with any command to get a list of supported arguments.

For more information on each module, information on dependencies and installation, and example commands, see the documentation at [https://aicjanelia.github.io/LLSM/](https://aicjanelia.github.io/LLSM/).


# Pipeline Overview

The pipeline is initiated by calling the `llsm-pipeline` or `mosaic-pipeline` command and providing it with a properly formatted `config.json` file. See an [example configuration file here](https://aicjanelia.github.io/LLSM/pipeline/config.html#example-configjson). We recommend using the `--dry-run` command to check your pipeline run before submitting jobs to the cluster.

### Command Line Examples
```
llsm-pipeline /aic/instruments/llsm/experimentFolder/config.json

mosaic-pipeline /aic/instruments/MOSAIC/experimentFolder/config.json
```

### AIC Microscope Parameters

| Microscope | Angle | &#956;m/pixel |
| ----- | ----- | -----|
| LLSM | 31.8&#176; | 0.104 |
| MOSAIC | -32.45&#176; = 147.55&#176; | 0.108 |

### Usage
```
usage: llsm-pipeline [-h] [--dry-run] [--verbose] input

Batch deskewing, deconvolution, and mip script for LLSM images.

positional arguments:
  input          path to configuration JSON file

optional arguments:
  -h, --help     show this help message and exit
  --dry-run, -d  execute without submitting any bsub jobs
  --verbose, -v  print details (including commands to bsub)
```