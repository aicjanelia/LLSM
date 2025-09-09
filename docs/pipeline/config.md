---
title: Configuration File
layout: default
parent: Pipeline Usage
nav_order: 2
---

# Configuration File
{: .no_toc }

The configuration file controls how the pipeline runs all requested modules. JSON formatting rules (e.g., using `{ }` and `,` appropriately) apply. If you work with an appropriate IDE (e.g., [Visual Studio Code](https://code.visualstudio.com/)), it can help highlight any errors you might have made in your JSON syntax.

One configuration file can be used to process multiple experiments. For documentation purposes, it is often ideal to have one `config.json` file for a given project that is stored with the rest of the project's files.

## Table of Contents
{: .no_toc .text-delta }

1. TOC
{:toc}

## Paths

### _root_

All configuration files need to include information on image paths. The `root` directory can contain multiple experiments to process. The pipeline starts by walking the `root` path. Any folder or subfolder that contains a `*Settings.txt` file will be processed by the pipeline.

### _processed.json_
Once a folder has been processed, it will be added to a new file, `processed.json`. This new json file is made in the `root` directory. Any directories that are listed in `processed.json` will be skipped during any future processing. This is useful if, for example, you want to analyze the first experiment in a project, but want to keep using the same `config.json` to process the rest of the project. All previously processed experiments will be skipped, saving time and money.  However, if you make a mistake and want to re-run any given folder, you will need to open `processed.json` and manually delete the entry that is associated with that folder.

### _psf_
PSFs are required for running the deconvolution module. In prior versions of the pipeline, this section was required regardless, but the current version only checks for the `psf` section if deconvolution is requested. Inside the `psf` section, there are two required subsections for standard deconvolution: `dir` and `laser`.  The directory `dir` is parsed as a subdirectory of `root` and should contain PSF images with their corresponding settings file. See [Deconvolution](https://aicjanelia.github.io/LLSM/decon/decon.html) for more about these files. The file names that correspond to each laser must be provided as name-value pairs in the `laser` subsection. The laser names must exactly match the values in the acquisition settings file (e.g., don't use 561 if the settings file uses 560). If using the option to deconvolve between deskewing (`decon-first`), the `laser` section is replaced by a `resampled` section with the same structure that points to PSF files that have been resampled in Z.

## BDV
The optional BDV section determines naming and saving conventions for the output files.

### *bdv_save*
To save the output files with simplified names compatible with programs such as BigStitcher, set `bdv_save` to `true`. The option defaults to `false` if the section is not provided. To learn more see [File Organization](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html).

### _overwrite_
When `overwrite` is set to `true`, any exisiting tif files in the folder will be overwritten by newly processed files. When `overwrite` is set to `false`, commands will not be generated to replace exisiting tif files. The one exception is that exisiting MIP files will be overwritten if their source file also must be recreated. This allows the pipeline to pick up part of the way through a folder, or re-run failed cluster jobs. This value defaults to `true` to be compatible with prior implementations of the pipeline that did not have this feature.

## Individual Modules
Individual modules are requested by adding their own section to the JSON file.  The [example json file](#example-configjson) requests flatfield correction, cropping, deskewing, deconvolution, and the generation of MIP files. More details on the parameters for each are provided in the discussion of each module, but a high-level overview is provided here.  If all modules are requested, they are run in the order of `flatfield > crop > deskew > decon`, with MIPs created at each stage as described in [MIP](https://aicjanelia.github.io/LLSM/mip/mip.html). This ordering is changed if the configuration file contain a section called `decon-first`, which creates commands that run deconovlution before deskewing. For more about this ordering, see [Deconvolution](https://aicjanelia.github.io/LLSM/decon/decon.html).

### _flatfield_
To use the flatfield correction, paths to an average darkfield image and one normalized flatfield image per channel must be included in the paths section. These inputs are described further in [Flatfield Inputs](https://aicjanelia.github.io/LLSM/flatfield/flatfield.html#flatfield-inputs). Adding the paths does not enable this module; the module is enabled by adding a flatfield section with the image bit-depth.

### _crop_
For each side of the image that cropping is desired, the number of pixels to remove from that side is a parameter. For example, `"cropTop": 10` removes 10 pixels from the top of the image. Any sides that are not provide are assumed to be zero. Other optional parameters are described further in [Cropping]().

### _deskew_
Deskewing is based on the xy-resolution and the step size of the images. The step size of the images is automatically parsed from the acquisition settings.txt file, but `xy-res` should be provided in &#956;m in the configuration file. The value of `fill` determines the values added to empty space created by the deskewing process, while `bit-depth` is 16 for our systems. If omitted, `angle` will default to the LLSM value of 31.8 degrees or the MOSAIC value of -32.45 degrees.

### _decon_
The value of n in `decon` is not related to the bsub command, but rather is the number of Richardson-Lucy iterations. Subtract will subtract a camera offset from all images; this value should generally be 100 for the AIC systems. Our systems have a `bit-depth` of 16.

### _decon-first_
The `decon-first` section of the configration file contains nested sections for decon and deskew that use the same input parameters as the isolated modules. This one section will generate commands that first deconvolve and then deskew the data (i.e., without needing to call the deskew module separately). Using this option is faster and requires less memory than running deconvolution on the desekwed images, but requires first resampling the PSF (see [Point Spread Function for Deconvolution](https://aicjanelia.github.io/LLSM/decon/psf.html)). There is no reason to use `decon-first` on objective-scanned images, as in this case `decon` alone (without `deskew`) is sufficient.

### _mip_
The true/false values in `mip` determine if projections will be made along the x, y, and/or z axes. Setting all values to true is recommended.

## bsub
The `bsub` section determines how jobs will be sent to the LSF cluster and is the section most specific to using the Janelia set up. A job is created for each individual tiff file, so thus there is one job for each time point and each channel.

### _job output_
If no output path, `o`, is specified, an email will be generated for every individual job, which corresponds to each individual tif, and can number in the thousands. This should be avoided! Setting `o` to `"/dev/null"` will result in no output being sent. If you are troubleshooting, you can specify a path to a file (e.g., `"/nrs/aic/instruments/llsm/pipeline-test/output.txt"`) that can be viewed as the processing progresses.

### _run times_
Two values, `We` and `W`, impact the run times of each individual job (i.e., each individual tiff file) and are specified in minutes. The estimated runtime, `We` is a guess of how long the files will take to process. This is generally short (approximately 10 min) on the LLSM, but can vary widely for MOSAIC data. The hard runtime limit, `W`, will stop your job at that time point, whether the job has completed or not. This keeps jobs with mistakes from running too long on the cluster and is currently set to a default of 8 hours. **If jobs are expected to run for longer than 8 hours, the value of `W` must be increased in the configuration file or jobs will not complete.**  If confident that jobs will take less than 1 hour each, setting `W` to 59 will lead to jobs being placed in a faster queue for cluster processing.

### _slots_
The value of `n` determines the number of slots requested on the cluster. The only purpose of this parameter is to guarantee the correct amount of memory for processing. If more slots than necessary for memory are requested, jobs will actually slow down, not speed up. *The modules are not written to take advantage of parallel processing.*

Each slot has 15 GB of memory. The maximal memory is used by the deconvolution module. To determine how much memory to request for deconvolution, calculate the total voxels in the final output image. If you are deconvolving after deskewing, this will be the voxels in the deskewed image, not the raw input images. An empirical equation for memory usage is `memory = (7.5E-8 * total voxels) - 1.39119`. Divide this memory value by 15 GB and round up to determine the number of slots. If the value is close to an integer value (e.g., 1.95), a cautious approach is to add 1 to the rounded value (e.g., 2-->3) to avoid small fluctuations in memory usage causing errors.

## Example config.json

```json
{
    "paths": {
        "root": "/aic/instruments/llsm/pipeline-test/",
        "psf": {
            "dir": "Calibration",
            "laser": {
                "560": "560_PSF.tif",
                "488": "488_PSF.tif"
            }
        },
        "flatfield": {
            "dir": "Calibration",
            "dark": "DarkAverage.tif",
            "laser": {
                "488": "I_N_488.tif",
                "560": "I_N_560.tif"
            }
        }
    },
    "bdv": {
        "bdv_save": true, 
        "overwrite": false
    },
    "flatfield": {
        "bit-depth": 16
    },
    "crop": {
        "cropTop": 10,
        "cropBottom": 0,
        "cropLeft": 15,
        "cropRight": 5,
        "cropFront": 100,
        "cropBack": 50
    },
    "deskew": {
        "xy-res": 0.104,
        "fill": 0.0,
        "bit-depth": 16
    },
    "decon": {
        "n": 5,
        "bit-depth": 16,
        "subtract": 100.0
    },
    "mip": {
        "x": true,
        "y": true,
        "z": true
    },
    "bsub": {
        "o": "/dev/null",
        "We": 10,
        "n": 4,
        "W": 480
    }
}
```

### Configuration for decon-first
To use `decon-first`,  create a nested structure and make sure to specified resampled PSF paths in the psf information.

```json
{
    "paths": {
        "root": "/aic/instruments/llsm/pipeline-test/",
        "psf": {
            "dir": "Calibration",
            "laser": {
                "560": "560_PSF.tif",
                "488": "488_PSF.tif"
            },
            "resampled": {
                "560": "560_resampled_PSF.tif",
                "488": "488_resampled_PSF.tif"
            }
        }
    },
    "bdv": {
        "bdv_save": true
    },
    "decon-first": {
        "deskew": {
            "xy-res": 0.104,
            "fill": 0.0,
            "bit-depth": 16
        },
        "decon": {
            "n": 5,
            "bit-depth": 16,
            "subtract": 100.0
        }
    },
    "mip": {
        "x": true,
        "y": true,
        "z": true
    },
    "bsub": {
        "o": "/dev/null",
        "We": 10,
        "n": 2,
        "W": 120
    }
}
```