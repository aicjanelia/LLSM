---
title: Pipeline Usage
layout: default
nav_order: 2
---

# Overview

This pipeline was designed to be used with the high performance computing cluster at Janelia Research Campus.  Using the overview pipeline commands requires that jobs can be submitted to an LSF cluster, but individual modules can always be used directly on the command line.  Additionally, the commands need to be on your path to be used directly. (For new AIC members, this requires a one-time set up for your cluster account.)

The main input into either `llsm-pipeline` or `mosaic-pipeline` is a configuration JSON file. This configuration file is a structured way of informing the pipeline of which modules will be used, what the relevant file paths are, and any necessary parameters. An example `config.json` file is provided in example directory of the [GitHub Repository](https://github.com/aicjanelia/LLSM) and further details about its organization are available the [Configuration File](https://aicjanelia.github.io/LLSM/pipeline/config.html) documentation.


### Use a dry run before submitting jobs
We recommend using the optional `--dry-run` (or `-d`) command before submitting jobs to the cluster. When this optional argument is passed to the pipeline command, the script will attempt to process the files without actually submitting any jobs to the cluster. The command line will display information about the requested processing that can be used to confirm that processing will proceed as desired.

The following dry run output confirms the path of files to be processed, that the files will be saved with the bdv naming format (see [File Organization](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html)), and that there is one combination of cameras and channels, which corresponds to the 488 nm laser.

```
processing 'full\path\to\folder`
parsing 'Scan_Settings.text'...
saving in bdv naming format...
scan type is tile
CamA_ch0=488
Done
```

For most cases, the `llsm-pipeline` \ `mosaic-pipeline` command should be all that is needed. However, the individual modules can be run directly on a single file if needed. See the sections for each module for information on running the modules separate from the pipeline.

# Usage

### Example command
```
llsm-pipeline -d /nrs/aic/instruments/llsm/pipeline-test/config.json
```

### LLSM Options

```
usage: llsm-pipeline [-h] [--dry-run] [--verbose] input

Batch processing script for LLSM images.

positional arguments:
  input          path to configuration JSON file

optional arguments:
  -h, --help     show this help message and exit
  --dry-run, -d  execute without submitting any bsub jobs
  --verbose, -v  print details (including commands to bsub)
```

### MOSAIC Options

```
usage: mosaic-pipeline [-h] [--dry-run] [--verbose] input

Batch processing script for MOSAIC images.

positional arguments:
  input          path to configuration JSON file

optional arguments:
  -h, --help     show this help message and exit
  --dry-run, -d  execute without submitting any bsub jobs
  --verbose, -v  print details (including commands to bsub)
```