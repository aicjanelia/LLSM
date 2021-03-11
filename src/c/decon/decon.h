#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include "itkRichardsonLucyDeconvolutionImageFilter.h"

#include "itkMacro.h"
#include "itkMath.h"

// Inverse:
//  Tikhonov
//  Wiener
// Iterative:
//  Landweber (results may have negative values)
//  Parametric Blind Least Squares
//  Projected Landweber
//  Richardson-Lucy


ImageType_g::Pointer decon_img RichardsonLucy(ImageType_g::Pointer img, ImageType_g kernel, unsigned int iterations, bool verbose) {

    using DeconFilterType = itk::RichardsonLucyDeconvolutionImageFilter< ImageType_g >;

    DeconFilterType::Pointer filter = DeconFilterType::New();
    filter->SetInput(img);
    filter->SetKernelImage(kernel);
    filter->NormalizeOn();
    filter->SetNumberOfIterations(iterations);

    return decon_filter->;
}