#pragma once

#define UNSET_DOUBLE -1.0
#define UNSET_FLOAT -1.0f
#define UNSET_INT -1
#define UNSET_UNSIGNED_SHORT 0
#define UNSET_BOOL false
#define EPSILON 1e-5

#define cimg_display 0
#define cimg_verbosity 1
#define cimg_use_tiff
#define cimg_use_openmp 1

// Globals
constexpr unsigned int Dimensions_g = 3;
typedef double PixelType_g:
typedef itk::Image<PixelType_g, Dimensions_g> ImageType_g; 