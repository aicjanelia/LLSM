---
title: Configuration File
layout: default
parent: Pipeline Usage
nav_order: 2
---

# Configuration File

The configuration file controls how the pipeline runs all requested modules. JSON formatting rules (e.g., using `{ }` and `,` appropriately) apply. If you work with an appropriate IDE (e.g., [Visual Studio Code](https://code.visualstudio.com/)), it can help highlight any errors you might have made in your JSON syntax.

One configuration file can be used to process multiple experiments. For documentation purposes, it is often ideal to have one `config.json` file for a given project that is stored with the rest of the project's files.

## Paths

### _root_

All configuration files need to include information on image paths. The `root` directory can contain multiple experiments to process. The pipeline starts by walking the `root` path. Any folder or subfolder that contains a `*Settings.txt` file will be processed by the pipeline.

### _processed.json_
Once a folder has been processed, it will be added to a new file, `processed.json`. This new json file is made in the `root` directory. Any directories that are listed in `processed.json` will be skipped during any future processing. This is useful if, for example, you want to analyze the first experiment in a project, but want to keep using the same `config.json` to process the rest of the project. All previously processed experiments will be skipped, saving time and money.  However, if you make a mistake and want to re-run any given folder, you will need to open `processed.json` and manually delete the entry that is associated with that folder.

### _psf_
PSFs are required for running the deconvolution module. In prior versions of the pipeline, this section was required regardless, but the current version only checks for the `psf` section if deconvolution is requested. Inside the `psf` section, there are two required subsections: `dir` and `laser`.  The directory `dir` is parsed as a subdirectory of `root` and should contain PSF images with their corresponding settings file. See [Deconvolution](https://aicjanelia.github.io/LLSM/decon/decon.html) for more about these files. The file names that correspond to each laser must be provided as name-value pairs in the `laser` subsection. The laser names must exactly match the values in the acquisition settings file (e.g., don't use 561 if the settings file uses 560).

## BDV
This optional section determines the naming convention for output files. It defaults to false if the section is not provided. To learn more see [File Organization](https://aicjanelia.github.io/LLSM/pipeline/bdv_save.html).

## Individual Modules
Individual modules are requested by adding their own section to the JSON file.  The [example json file](#example-configjson) requests cropping, deskewing, deconvolution, and the generation of MIP files. More details on the parameters for each are provided in the discussion of each module, but a high-level overview is provided here.  If all modules are requested, they are run in the order of `crop > deskew > decon`, with mips created at each stage as appropriate.

### _crop_
For each side of the image that cropping is desired, the number of pixels to remove from that side is a parameter. For example, `"cropTop": 10` removes 10 pixels from the top of the image. Any sides that are not provide are assumed to be zero. Other optional parameters are described further in [Cropping]().

### _deskew_
Deskewing is based on the xy-resolution and the step size of the images. The step size of the images is automatically parsed from the acquisition settings.txt file, but `xy-res` should be provided in &#956;m in the configuration file. The value of `fill` determines the values added to empty space created by the deskewing process, while `bit-depth` is 16 for our systems. If omitted, `angle` will default to the LLSM value of 31.8 degrees or the MOSAIC value of -32.45 degrees.

### _decon_
The value of n in `decon` is not related to the bsub command, but rather is the number of Richardson-Lucy iterations. Subtract will subtract a camera offset from all images; this value should generally be 100 for the AIC systems. Our systems have a `bit-depth` of 16.

### _mip_
The true/false values in `mip` determine if projections will be made along the x, y, and/or z axes. Setting all values to true is recommended.

## Bsub
The `bsub` section determines how jobs will be sent to the LSF cluster and is the section most specific to using the Janelia set up. A job is created for each individual tiff file, so thus there is one job for each timepoint and each channel.

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
        "root": "/nrs/aic/instruments/llsm/pipeline-test/",
        "psf": {
            "dir": "20210222/Calibration",
            "laser": {
                "560": "560_PSF.tif",
                "488": "488_PSF.tif"
            }
        }
    },
    "bdv": {
        "bdv_save": true
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