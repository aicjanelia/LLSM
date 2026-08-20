#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include "fft_size.h"
#include <itkImage.h>
#include <itkRichardsonLucyDeconvolutionImageFilter.h>
#include <itkProjectedLandweberDeconvolutionImageFilter.h>
#include <itkParametricBlindLeastSquaresDeconvolutionImageFilter.h>
#include <itkZeroFluxNeumannBoundaryCondition.h>
#include <itkMultiThreaderBase.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

template <class TDeconFilter>
void ConfigureFFTPadding(typename TDeconFilter::Pointer filter,
                         kImageType::Pointer img,
                         kImageType::Pointer kernel,
                         bool verbose=false)
{
    const kImageType::SizeType image_size = img->GetLargestPossibleRegion().GetSize();
    const kImageType::SizeType kernel_size = kernel->GetLargestPossibleRegion().GetSize();
    llsm::FFTSize image_fft_size{};
    llsm::FFTSize kernel_fft_size{};

    for (std::size_t dimension = 0; dimension < llsm::kFFTDimensions; ++dimension)
    {
        image_fft_size[dimension] = image_size[dimension];
        kernel_fft_size[dimension] = kernel_size[dimension];
    }

    const auto plan = llsm::PlanFFTPadding(image_fft_size,
                                           kernel_fft_size,
                                           filter->GetSizeGreatestPrimeFactor());

    if (!plan.minimum_size_supported)
    {
        std::ostringstream message;
        message << "decon: minimum FFT volume is " << plan.minimum_voxel_count
                << " voxels, exceeding the supported limit of "
                << llsm::kMaximumFFTVoxelCount
                << "; crop or split the input stack before deconvolution";
        throw std::runtime_error(message.str());
    }

    if (plan.disable_extra_padding)
    {
        // ITK normally rounds every dimension up to an FFT-friendly value. For
        // volumes close to INT32_MAX that optional rounding can cross ITK's
        // allocation limit even though the required convolution volume fits.
        filter->SetSizeGreatestPrimeFactor(0);
        std::cerr << "decon: disabling optional FFT size rounding ("
                  << plan.optimized_voxel_count << " voxels would exceed "
                  << llsm::kMaximumFFTVoxelCount << "; using "
                  << plan.minimum_voxel_count << " voxels)" << std::endl;
    }
    else if (verbose)
    {
        std::cout << "FFT padded voxel count = " << plan.optimized_voxel_count << std::endl;
    }
}

// Richardson-Lucy
// Requires a kernel and a number of iterations.
kImageType::Pointer RichardsonLucy(kImageType::Pointer img, kImageType::Pointer kernel, unsigned int iterations, bool verbose=false)
{
    using DeconFilterType = itk::RichardsonLucyDeconvolutionImageFilter<kImageType>;
    itk::ZeroFluxNeumannBoundaryCondition< kImageType > bc;

    // Enable FFTW multi-threading if available
    #if defined(ITK_USE_FFTWF) || defined(ITK_USE_FFTWD)
        itk::FFTWGlobalConfiguration::SetPlanRigor(FFTW_MEASURE);
        //itk::FFTWGlobalConfiguration::SetPlanRigor(FFTW_ESTIMATE);
        //itk::FFTWGlobalConfiguration::SetReadWisdomCache(false);
        //itk::FFTWGlobalConfiguration::SetWriteWisdomCache(false);
        
        if (verbose) {
            unsigned int num_threads = itk::MultiThreaderBase::GetGlobalDefaultNumberOfThreads();
            std::cout << "FFTW backend detected - attempting multi-threaded deconvolution with " 
                      << num_threads << " threads" << std::endl;
            std::cout << "FFTW wisdom cache is disabled" << std::endl;
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
    ConfigureFFTPadding<DeconFilterType>(filter, img, kernel, verbose);
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
    ConfigureFFTPadding<DeconFilterType>(filter, img, kernel, verbose);
    filter->NormalizeOn();
    filter->SetNumberOfIterations(iterations);
    filter->SetAlpha(alpha);
    filter->SetOutputRegionModeToSame();
    filter->SetBoundaryCondition(&bc);
    filter->Update(); 

    return filter->GetOutput();
}
