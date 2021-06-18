#pragma once

#include "defines.h"

#include <itkResampleImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>

// TODO make this templated
kImageType::Pointer Resampler(kImageType::Pointer image, kImageType::SpacingType out_spacing, bool verbose=false)
{
  // set up resample filter
  using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
  FilterType::Pointer filter = FilterType::New();

  using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>;
  InterpolatorType::Pointer interpolator = InterpolatorType::New();
  filter->SetInterpolator(interpolator);
  filter->SetOutputSpacing(out_spacing);

  // calculate size based on spacing
  kImageType::SpacingType in_spacing = image->GetSpacing();
  kImageType::SizeType in_size = image->GetLargestPossibleRegion().GetSize();
  kImageType::SizeType size;
  for (int i=0; i<kDimensions; ++i) // TODO figure out the dimensions from the type
  {
    size[i] = in_size[i] * in_spacing[i] / out_spacing[i];
  }
  filter->SetSize(size);

  // perform resample
  filter->SetInput(image);
  filter->Update();

  if (verbose)
  {
    printf("In spacing: ");
    for (int i=0; i<kDimensions; ++i)
    {
      printf("%0.3f ", in_spacing[i]);
    }

    printf("\nOut spacing: ");
    for (int i=0; i<kDimensions; ++i)
    {
      printf("%0.3f ", out_spacing[i]);
    }

    printf("\nOut dimension: ");
    for (int i=0; i<kDimensions; ++i)
    {
      printf("%ld ", size[i]);
    }

    printf("\n");
  }

  return filter->GetOutput();
}
