#pragma once

#define MIP_VERSION "AIC MIP version 0.1.0"

#include "defines.h"
#include <itkImage.h>
#include <itkImageBase.h>

#include <itkMaximumProjectionImageFilter.h>

#include <itkMultiThreaderBase.h>

itk::Image<kPixelType, 2>::Pointer MaxIntensityProjection(kImageType::Pointer img, unsigned int axis, bool verbose=false)
{
  using FilterType = itk::MaximumProjectionImageFilter<kImageType, itk::Image<kPixelType, 2>>;
  // using FilterType = itk::MaximumProjectionImageFilter<kImageType, kImageType>;

  FilterType::Pointer filter = FilterType::New();
  filter->SetInput(img);
  filter->SetProjectionDimension(axis);
  filter->Update();
  itk::Image<kPixelType, 2>::Pointer img_out = filter->GetOutput();

  kImageType::SpacingType spacing_in = img->GetSpacing();
  itk::Image<kPixelType, 2>::SpacingType spacing_out;

  // assuming isometric dimensions
  spacing_out[0] = spacing_in[0]; //TODO: could be dangerous to assume
  spacing_out[1] = spacing_in[0];
  img_out->SetSpacing(spacing_out);

  return img_out;
}
