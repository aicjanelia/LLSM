#!/usr/bin/env python

# Usage: resampleZ input_file output_file scale
#
# Dependencies: ITK
# (https://docs.itk.org/en/latest/learn/python_quick_start.html)
#
# This script allows you to resample an image stack in the z-dimension.
# The input file is a single tif file containing the whole z-stack.
# The output file will be the same size in x and y, but will be adjusted
# for the new size in z after scaling.
# The scale variable will shink (> 1) or enlarge (< 1) the z-dimension.
#
# As an example, if a PSF was acquired with a step size of 0.1 and the data
# was acquired with a stage scanning step of 0.4 on the MOSAIC, you may want
# to rescale the PSF to have a step size of 0.4*sin(32.45 degrees) ~= 0.21.
# In this case, scale = 0.4*sin(32.45 degrees)/0.1 ~= 2.14625362388 will result
# in a properly rescaled PSF. If the input PSF has a size of 25x25x25 pixels,
# the output PSF will have a size of 25x25x11 pixels.

import sys
import itk
import math

# Read in the system arguments
if len(sys.argv) != 4:
    print("Usage: " + sys.argv[0] + " <input_image> <output_image> <scale>")
    sys.exit(1)

input_file_name = sys.argv[1]
output_file_name = sys.argv[2]
scale = float(sys.argv[3])

# Read in the input image
input_image = itk.imread(input_file_name)
input_size = itk.size(input_image)
input_spacing = itk.spacing(input_image)
input_origin = itk.origin(input_image)
Dimension = input_image.GetImageDimension()

# Set up the output image
output_size = [input_size[0], input_size[1], math.floor(input_size[2]/scale)]
output_spacing = input_spacing
output_origin = input_origin
# Get the parameters for the rescaling
scale_transform = itk.ScaleTransform[itk.D, Dimension].New()
scale_transform_parameters = scale_transform.GetParameters()
scale_transform_parameters[2] = scale #only change z
scale_transform_center = [float(int(s / 2)) for s in input_size]
scale_transform_center[2] = scale_transform_center[2] - input_size[2] + output_size[2] # shift it?
scale_transform.SetParameters(scale_transform_parameters)
scale_transform.SetCenter(scale_transform_center)

# Set up linear interpolation
interpolator = itk.LinearInterpolateImageFunction.New(input_image)
# Resample the file appropriately
resampled = itk.resample_image_filter(
    input_image,
    transform=scale_transform,
    interpolator=interpolator,
    size=output_size,
    output_spacing=output_spacing,
    output_origin=output_origin,
)

# Save the final output
itk.imwrite(resampled, output_file_name)