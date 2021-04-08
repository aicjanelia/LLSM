#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include <itkImage.h>
#include <itkRichardsonLucyDeconvolutionImageFilter.h>
#include <itkProjectedLandweberDeconvolutionImageFilter.h>
#include <itkParametricBlindLeastSquaresDeconvolutionImageFilter.h>
#include <itkZeroFluxNeumannBoundaryCondition.h>


// Inverse:
//  Tikhonov
//  Wiener
// Iterative:
//  Landweber (results may have negative values)
//  Parametric Blind Least Squares
//  Projected Landweber
//  Richardson-Lucy

// Iterative Methods

// Richardson-Lucy
// Requires a kernel and a number of iterations.
itk::SmartPointer<kImageType> RichardsonLucy(itk::SmartPointer<kImageType> img, itk::SmartPointer<kImageType> kernel, unsigned int iterations, bool verbose=false)
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
    filter->Update();

    return filter->GetOutput();
}

// Projected Landweber (non-negative version of Landweber)
// Requires a kernel, a number of iterations, and a relaxation parameter alpha. The parameter alpha is positive and less than 2/sigma1^2, where 
// sigma1 is the largest singular value of the convolution operator.
itk::SmartPointer<kImageType> ProjectedLandweber(itk::SmartPointer<kImageType> img, itk::SmartPointer<kImageType> kernel, unsigned int iterations, double alpha, bool verbose=false)
{
    using DeconFilterType = itk::ProjectedLandweberDeconvolutionImageFilter<kImageType>;
    itk::ZeroFluxNeumannBoundaryCondition< kImageType > bc;

    DeconFilterType::Pointer filter = DeconFilterType::New();
    filter->SetInput(img);
    filter->SetKernelImage(kernel);
    filter->NormalizeOn();
    filter->SetNumberOfIterations(iterations);
    filter->SetAlpha(alpha);
    filter->SetOutputRegionModeToSame();
    filter->SetBoundaryCondition(&bc);
    filter->Update(); 

    return filter->GetOutput();
}
