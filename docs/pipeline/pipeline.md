---
title: Pipeline Usage
layout: default
nav_order: 2
---

# Overview

This pipeline was designed to be used with the high performance computing cluster at Janelia Research Campus.  Using the overview pipeline commands requires that jobs can be submitted to an LSF cluster, but individual modules can always be used directly on the command line.  Additionally, the commands need to be on your path to be used directly. (For new AIC members, this requires a one-time set up for your cluster account.)

The main input into either `llsm-pipeline` or `mosaic-pipeline` is a configuration JSON file. This configuration file is a structured way of informing the pipeline of which modules will be used, what the relevant file paths are, and any necessary parameters.

## Usage

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

The pipeline is initiated by calling the `llsm-pipeline` command and providing it with a properly formatted `config.json` file. See the `example` directory of this repo for an example `config.json`. We recommend using the `--dry-run` command to check your pipeline run before submitting jobs to the cluster.

For most cases, the `llsm-pipeline` command should be all that is needed. However, the individual modules can be run directly on a single file if needed. See the sections before for information on running the modules separate from the pipeline.