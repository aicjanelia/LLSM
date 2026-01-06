#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include <itkImage.h>
#include <itkRichardsonLucyDeconvolutionImageFilter.h>
#include <itkProjectedLandweberDeconvolutionImageFilter.h>
#include <itkParametricBlindLeastSquaresDeconvolutionImageFilter.h>
#include <itkZeroFluxNeumannBoundaryCondition.h>
#include <itkMultiThreaderBase.h>
#include <iostream>

// FFTW support for multi-threading
#if defined(ITK_USE_FFTWF) || defined(ITK_USE_FFTWD)
  #include <itkFFTWGlobalConfiguration.h>
  #ifdef ITK_USE_FFTWF
    #include <itkFFTWForwardFFTImageFilter.h>
  #endif
  #ifdef ITK_USE_FFTWD
    #include <itkFFTWForwardFFTImageFilter.h>
  #endif
#endif

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
kImageType::Pointer RichardsonLucy(kImageType::Pointer img, kImageType::Pointer kernel, unsigned int iterations, bool verbose=false)
{
    using DeconFilterType = itk::RichardsonLucyDeconvolutionImageFilter<kImageType>;
    itk::ZeroFluxNeumannBoundaryCondition< kImageType > bc;

    // Enable FFTW multi-threading if available
    #if defined(ITK_USE_FFTWF) || defined(ITK_USE_FFTWD)
        itk::FFTWGlobalConfiguration::SetPlanRigor(FFTW_MEASURE);
        
        if (verbose) {
            unsigned int num_threads = itk::MultiThreaderBase::GetGlobalDefaultNumberOfThreads();
            std::cout << "FFTW backend detected - attempting multi-threaded deconvolution with " 
                      << num_threads << " threads" << std::endl;
        }
    #else
        if (verbose) {
            std::cout << "Warning: FFTW not available, using single-threaded VNLFFT backend" << std::endl;
            std::cout << "For multi-threaded deconvolution, rebuild ITK with FFTW support" << std::endl;
        }
    #endif

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
kImageType::Pointer ProjectedLandweber(kImageType::Pointer img, kImageType::Pointer kernel, unsigned int iterations, double alpha, bool verbose=false)
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
