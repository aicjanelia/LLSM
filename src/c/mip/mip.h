#pragma once

#define MIP_VERSION "AIC MIP version 0.1.0"

#include "defines.h"
#include <itkImage.h>
#include <itkImageBase.h>

#include "itkMaximumProjectionImageFilter.h"

itk::Image<kPixelType, 2>::Pointer MaxIntensityProjection(kImageType::Pointer img, unsigned int axis, bool verbose=false)
{
  using FilterType = itk::MaximumProjectionImageFilter<kImageType, itk::Image<kPixelType, 2>>;
  // using FilterType = itk::MaximumProjectionImageFilter<kImageType, kImageType>;

  FilterType::Pointer filter = FilterType::New();
  filter->SetInput(img);
  filter->SetProjectionDimension(axis);
  filter->Update();
  return filter->GetOutput();
}