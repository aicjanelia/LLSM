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
#include <itkMultiThreaderBase.h>

kImageType::Pointer Deskew(kImageType::Pointer img, float angle, float step, float xy_res, kPixelType fill_value, bool verbose=false)
{
  img->SetSpacing((1.0, 1.0, 1.0));

  // compute shift
  const double shift = step * cos(angle * M_PI/180.0) / xy_res;

  if (verbose)
  {
    std::cout << "\nDeskew Parameters\n";
    std::cout << "Shift (px) = " << shift << "\n";
  }

  // calculate and set output size
  kImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();

  if (verbose)
  {
    std::cout << "Input Dimensions (px) = " << size[0] << " x " << size[1] << " x " << size[2] << "\n";
  }

  int orgx = size[0];
  size[0] = ceil(size[0] + (fabs(shift) * (size[2]-1)));

  if (verbose)
  {
    std::cout << "Output Dimensions (px) = " << size[0] << " x " << size[1] << " x " << size[2] << "\n";
  }

  // set up resample filter
  using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
  FilterType::Pointer filter = FilterType::New();

  using TransformType = itk::AffineTransform<double, kDimensions>;
  TransformType::Pointer transform = TransformType::New();
  transform->Shear(0, 2, -shift);
  if (shift < 0)
  {
    int transx = size[0] - orgx;
    TransformType::OutputVectorType translation;
    translation[0] = -transx; // X translation
    translation[1] = 0; // Y translation
    translation[2] = 0; // Z translation
    transform->Translate(translation);
  }
  filter->SetTransform(transform);

  using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>;
  InterpolatorType::Pointer interpolator = InterpolatorType::New();
  filter->SetInterpolator(interpolator);

  filter->SetDefaultPixelValue(fill_value);

  filter->SetSize(size);

  // perform deskew
  filter->SetInput(img);
  filter->Update();

  kImageType::Pointer outimg = filter->GetOutput();

  // set spacing
  kImageType::SpacingType spacing;
  spacing[0] = xy_res;
  spacing[1] = xy_res;
  spacing[2] = fabs(step * sin(angle * M_PI/180.0));

  img->SetSpacing(spacing);

  if (verbose)
  {
    std::cout << "Output Resolution (um/px) = " << spacing[0] << " x " << spacing[1] << " x " << spacing[2] << std::endl;
  }

  return outimg;
}