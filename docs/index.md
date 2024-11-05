---
title: Home
layout: home
nav_order: 1
---

The Advanced Imaging Center (AIC) uses this pipeline for pre-processing of images created on our LLSM or MOSAIC systems. The pipeline consists of multiple discrete modules that are accessed via the command line using either the `llsm-pipeline` or `mosaic-pipeline` commands. The main difference in the two pipelines is the parsing of filenames and acquisition settings metadata.

Each of the modules compiles to a separate binary that can also be directly executed by calling it on the command line. These modules are:
- Cropping
- Deskewing
- Deconvolution
- Maximum Intensity Projection

The main pipeline command and each individual module are further described in this documentation. With any command, you can also use the `-h` option to get a list of supported arguments.