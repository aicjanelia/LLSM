#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include <itkImage.h>
#include <itkRichardsonLucyDeconvolutionImageFilter.h>
#include <itkZeroFluxNeumannBoundaryCondition.h>


// Inverse:
//  Tikhonov
//  Wiener
// Iterative:
//  Landweber (results may have negative values)
//  Parametric Blind Least Squares
//  Projected Landweber
//  Richardson-Lucy


itk::SmartPointer<kImageType> RichardsonLucy(kImageType::Pointer img, kImageType::Pointer kernel, unsigned int iterations, bool verbose)
{
    using DeconFilterType = itk::RichardsonLucyDeconvolutionImageFilter<kImageType>;
    itk::ZeroFluxNeumannBoundaryCondition< kImageType > bc;

    DeconFilterType::Pointer filter = DeconFilterType::New();
    filter->SetInput(img);
    filter->SetKernelImage(kernel);
    filter->NormalizeOn();
    filter->SetNumberOfIterations(iterations);
    filter->SetOutputRegionModeToSame();
    filter->SetBoundaryCondition(&bc);

    return filter->GetOutput();
}