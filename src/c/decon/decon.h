#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include "itkFFTConvolutionImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkLandweberDeconvolutionImageFilter.h"
#include "itkCastImageFilter.h"
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


int RichardsonLucy(bool verbose)
{
    typedef float                                               RealPixelType;
    typedef unsigned char                                       UCharPixelType;
    const unsigned int                                          Dimension = 2;
    typedef itk::Image< RealPixelType, Dimension >              ImageType;
    typedef itk::Image<UCharPixelType, Dimension >              UCharImageType;
    typedef itk::ImageFileReader< ImageType >                   ReaderType;
    typedef itk::ImageFileWriter<UCharImageType >               WriterType;
    typedef itk::FFTConvolutionImageFilter< ImageType >         ConvolutionFilterType;
    typedef itk::LandweberDeconvolutionImageFilter< ImageType > DeconvolutionFilterType;
    typedef itk::CastImageFilter<ImageType, UCharImageType>     CastFilterType;

    ReaderType::Pointer inputReader = ReaderType::New();
    inputReader->SetFileName(argv[1]);
    inputReader->Update();

    ReaderType::Pointer kernelReader = ReaderType::New();
    kernelReader->SetFileName(argv[2]);
    kernelReader->Update();

    // Generate a convolution of the input image with the kernel image
    ConvolutionFilterType::Pointer convolutionFilter = ConvolutionFilterType::New();
    convolutionFilter->SetInput(inputReader->GetOutput());
    convolutionFilter->NormalizeOn();
    convolutionFilter->SetKernelImage(kernelReader->GetOutput());

    // Test the deconvolution algorithm
    DeconvolutionFilterType::Pointer deconvolutionFilter = DeconvolutionFilterType::New();
    deconvolutionFilter->SetInput(convolutionFilter->GetOutput());
    deconvolutionFilter->SetKernelImage(kernelReader->GetOutput());
    deconvolutionFilter->NormalizeOn();
    deconvolutionFilter->SetAlpha(atof(argv[6]));
    if (itk::Math::NotExactlyEquals(deconvolutionFilter->GetAlpha(), atof( argv[6] ))) {
        std::cerr << "Set/GetAlpha() test failed." << std::endl;
        return EXIT_FAILURE;
    }

    unsigned int iterations = static_cast< unsigned int >(atoi(argv[5]));
    deconvolutionFilter->SetNumberOfIterations(iterations);

    CastFilterType::Pointer castOutputFilter = CastFilterType::New();
    castOutputFilter->SetInput(deconvolutionFilter->GetOutput());

    CastFilterType::Pointer castInputFilter = CastFilterType::New();
    castInputFilter->SetInput(convolutionFilter->GetOutput());

    // Write the deconvolution result
    try {
        WriterType::Pointer writer = WriterType::New();

        writer->SetFileName(argv[3]);
        writer->SetInput(castInputFilter->GetOutput());
        writer->Update();

        writer->SetFileName(argv[4] );
        writer->SetInput(castOutputFilter->GetOutput());
        writer->Update();
    } catch ( itk::ExceptionObject & e ) {
        std::cerr << "Unexpected exception caught when writing deconvolution image: "
            << e << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}