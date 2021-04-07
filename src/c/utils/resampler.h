#include "defines.h"

#include <itkResampleImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>

// TODO make this templated
itk::SmartPointer<kImageType> Resampler(itk::SmartPointer<kImageType> image, kImageType::SpacingType out_spacing)
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

  return filter->GetOutput();
}
