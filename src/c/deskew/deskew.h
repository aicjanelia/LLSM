#pragma once

#define DESKEW_VERSION "AIC Deskew version 0.1.0"
#define _USE_MATH_DEFINES

#include "defines.h"
#include <cmath>
#include <itkImage.h>
#include <itkImageBase.h>
#include <itkResampleImageFilter.h>
#include "itkAffineTransform.h"
#include "itkLinearInterpolateImageFunction.h"

kImageType::Pointer Deskew(kImageType::Pointer img, float angle, float step, float xy_res, kPixelType fill_value, bool verbose=false)
{
  // compute shift
  const double shift = step * cos(angle * M_PI/180.0) / xy_res;

  if (verbose)
  {
    std::cout << "\nDeskew Parameters\n";
    std::cout << "Shift (px) = " << shift << "\n";
  }

  // set up resample filter
  using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
  FilterType::Pointer filter = FilterType::New();

  using TransformType = itk::AffineTransform<double, kDimensions>;
  TransformType::Pointer transform = TransformType::New();
  transform->Shear(0, 2, -shift);
  filter->SetTransform(transform);

  using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>;
  InterpolatorType::Pointer interpolator = InterpolatorType::New();
  filter->SetInterpolator(interpolator);

  filter->SetDefaultPixelValue(fill_value);

  // calculate and set output size
  kImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();

  if (verbose)
  {
    std::cout << "Input Dimensions (px) = " << size[0] << " x " << size[1] << " x " << size[2] << "\n";
  }

  size[0] = ceil(size[0] + (fabs(shift) * (size[2]-1)));

  if (verbose)
  {
    std::cout << "Output Dimensions (px) = " << size[0] << " x " << size[1] << " x " << size[2] << "\n";
  }

  filter->SetSize(size);

  // perform deskew
  filter->SetInput(img);
  filter->Update();

  // set spacing
  kImageType::SpacingType spacing;

  spacing[0] = xy_res;
  spacing[1] = xy_res;
  spacing[2] = step * sin(angle * M_PI/180.0);

  img->SetSpacing(spacing);

  if (verbose)
  {
    std::cout << "Output Resolution (um/px) = " << spacing[0] << " x " << spacing[1] << " x " << spacing[2] << std::endl;
  }

  return filter->GetOutput();
}