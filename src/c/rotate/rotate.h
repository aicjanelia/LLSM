#pragma once

#define ROTATE_VERSION "AIC Rotate version 0.1.0"

#include "defines.h"
#include "rotate_geometry.h"

#include <iostream>
#include <itkAffineTransform.h>
#include <itkImage.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkMatrix.h>
#include <itkResampleImageFilter.h>

// Rotates a (typically deskewed) volume about the y axis in physical space with a single
// linear-interpolation resampling pass. See rotate_geometry.h for the angle convention,
// the output grid and the crop modes.
//
//   angle       degrees, BigStitcher "y-axis rotation" convention (MOSAIC: -32.45)
//   xy_res      x/y spacing of the input (um/px)
//   z_res       z spacing of the input (um/px), i.e. step * sin(angle) after deskewing
//   shift       signed deskew shift in px per z plane (0 = treat the whole array as valid data)
//   fill_value  value for output voxels outside the input
//   z_trim      output z slices to remove from the {bottom, top} ends of the cropped output
//
// Throws std::invalid_argument for unusable parameters (non-positive spacing, strict crop
// with an angle that does not match the deskew geometry, ...).
inline kImageType::Pointer Rotate(kImageType::Pointer img,
                                  double angle,
                                  double xy_res,
                                  double z_res,
                                  double shift,
                                  llsm_rotate::CropMode mode,
                                  bool isotropic,
                                  kPixelType fill_value,
                                  bool verbose = false,
                                  std::array<long, 2> z_trim = {0, 0})
{
  if (!(xy_res > 0.0) || !(z_res > 0.0))
    throw std::invalid_argument("rotate requires positive x/y and z spacing");

  // Physical frame: origin at input voxel (0,0,0), identity direction, true spacing.
  kImageType::SpacingType in_spacing;
  in_spacing[0] = xy_res;
  in_spacing[1] = xy_res;
  in_spacing[2] = z_res;
  kImageType::PointType zero_origin;
  zero_origin.Fill(0.0);
  kImageType::DirectionType identity;
  identity.SetIdentity();
  img->SetSpacing(in_spacing);
  img->SetOrigin(zero_origin);
  img->SetDirection(identity);

  const kImageType::SizeType in_size = img->GetLargestPossibleRegion().GetSize();
  const llsm_rotate::OutputGrid grid = llsm_rotate::ComputeOutputGrid(
      {in_size[0], in_size[1], in_size[2]}, {xy_res, xy_res, z_res}, angle, shift, mode, isotropic, z_trim);

  // ITK maps output points to input points, so apply the inverse (transpose) of the content rotation.
  const auto r = llsm_rotate::RotationMatrix(angle);
  using TransformType = itk::AffineTransform<double, kDimensions>;
  TransformType::Pointer transform = TransformType::New();
  TransformType::MatrixType inverse;
  for (unsigned int row = 0; row < 3; ++row)
    for (unsigned int col = 0; col < 3; ++col)
      inverse[row][col] = r[col * 3 + row];
  transform->SetMatrix(inverse);

  kImageType::SpacingType out_spacing;
  kImageType::PointType out_origin;
  kImageType::SizeType out_size;
  for (unsigned int d = 0; d < 3; ++d)
  {
    out_spacing[d] = grid.spacing[d];
    out_origin[d] = grid.origin[d];
    out_size[d] = grid.size[d];
  }

  if (verbose)
  {
    std::cout << "\nRotate Parameters\n";
    std::cout << "Angle (degrees) = " << angle << "\n";
    std::cout << "Deskew shift (px/plane) = " << shift << "\n";
    std::cout << "Input Dimensions (px) = " << in_size[0] << " x " << in_size[1] << " x " << in_size[2] << "\n";
    std::cout << "Input Resolution (um/px) = " << in_spacing[0] << " x " << in_spacing[1] << " x " << in_spacing[2] << "\n";
    std::cout << "Output Dimensions (px) = " << out_size[0] << " x " << out_size[1] << " x " << out_size[2] << "\n";
    std::cout << "Output Resolution (um/px) = " << out_spacing[0] << " x " << out_spacing[1] << " x " << out_spacing[2] << "\n";
    std::cout << "Output grid starts at index (" << grid.first[0] << ", " << grid.first[1] << ", " << grid.first[2] << ")" << std::endl;
  }

  using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
  FilterType::Pointer filter = FilterType::New();
  filter->SetTransform(transform);

  using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>;
  filter->SetInterpolator(InterpolatorType::New());

  filter->SetDefaultPixelValue(fill_value);
  filter->SetOutputSpacing(out_spacing);
  filter->SetOutputOrigin(out_origin);
  filter->SetOutputDirection(identity);
  filter->SetSize(out_size);

  filter->SetInput(img);
  filter->Update();

  return filter->GetOutput();
}
