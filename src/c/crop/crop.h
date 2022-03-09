#pragma once

#define CROP_VERSION "AIC crop version 0.1.0"

#include "defines.h"
#include <cmath>
#include <itkImage.h>
#include <itkImageBase.h>
#include <itkExtractImageFilter.h>

kImageType::Pointer Crop(kImageType::Pointer img, float z_step, float xy_res, int top, int bottom, int left, int right, bool verbose=false)
{
    // calculate and set output size
    kImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();

    if (verbose)
    {
        std::cout << "Input Dimensions (px) = " << size[0] << " x " << size[1] << " x " << size[2] << "\n";
        std::cout << "Output Dimensions (px) = " << size[0] - left - right << " x " << size[1] - top - bottom << " x " << size[2] << "\n";
    }
    
    kImageType::IndexType desiredStart;
    desiredStart.SetElement(0, left);
    desiredStart.SetElement(1, top);
    desiredStart.SetElement(2, 0);

    kImageType::SizeType desiredSize;
    desiredSize.SetElement(0, size[0] - left - right);
    desiredSize.SetElement(1, size[1] - top - bottom);
    desiredSize.SetElement(2, size[2]);

    kImageType::RegionType desiredRegion(desiredStart, desiredSize);

    if (verbose)
        std::cout << "desiredRegion: " << desiredRegion << std::endl;

    using FilterType = itk::ExtractImageFilter<kImageType, kImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetExtractionRegion(desiredRegion);
    filter->SetInput(img);
    #if ITK_VERSION_MAJOR >= 4
    filter->SetDirectionCollapseToIdentity(); // This is required.
    #endif
    filter->Update();

    kImageType::Pointer outimg = filter->GetOutput();

    // set spacing
    //kImageType::SpacingType spacing;

    //spacing[0] = xy_res;
    //spacing[1] = xy_res;
    //spacing[2] = z_step;

    //outimg->SetSpacing(spacing);

    return outimg;
}