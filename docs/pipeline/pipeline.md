---
title: Pipeline Usage
layout: default
nav_order: 2
---

## Pipeline

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

The pipeline is initiated by calling the `llsm-pipeline` command and providing it with a properly formatted `config.json` file. See the `example` directory of this repo for an example `config.json`. We recommend using the `--dry-run` command to check your pipeline run before submitting jobs to the cluster.

For most cases, the `llsm-pipeline` command should be all that is needed. However, the individual modules can be run directly on a single file if needed. See the sections before for information on running the modules separate from the pipeline.