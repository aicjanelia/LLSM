#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>
#include <tiffio.hxx>

#include <itkImage.h>
#include <itkCastImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

template <class TImage>
void GetRange(itk::SmartPointer<TImage> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<unsigned char, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = 255.0;
}

void GetRange(itk::SmartPointer<itk::Image<char, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = -128.0;
  max_val = 127.0;
}

void GetRange(itk::SmartPointer<itk::Image<unsigned short, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = pow(2.0, 16.0) - 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<short, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = pow(2.0, 15.0) * -1.0;
  max_val = pow(2.0, 15.0) - 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<unsigned int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = pow(2.0, 32.0) - 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = pow(2.0, 31.0) * -1.0;
  max_val = pow(2.0, 31.0) - 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<unsigned long, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = pow(2.0, 64.0) - 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<long, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = pow(2.0, 63.0) * -1.0;
  max_val = pow(2.0, 63.0) - 1.0;
}

template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ConvertImage(itk::SmartPointer<TImageIn> image_in)
{
  double input_min, input_max, output_min, output_max;

  GetRange(image_in, input_min, input_max);
  typename itk::SmartPointer<TImageOut> image_out = TImageOut::New();
  GetRange(image_out, output_min, output_max);

  using RescaleType = itk::IntensityWindowingImageFilter<TImageIn, TImageOut>;
  typename RescaleType::Pointer convert_fltr = RescaleType::New();
  convert_fltr->SetInput(image_in);
  convert_fltr->SetWindowMinimum(input_min);
  convert_fltr->SetWindowMaximum(input_max);
  convert_fltr->SetOutputMinimum(output_min);
  convert_fltr->SetOutputMaximum(output_max);
  convert_fltr->Update();

  return convert_fltr->GetOutput();
}

template <class TImage>
itk::SmartPointer<TImage> ConvertImage(itk::SmartPointer<TImage> image_in)
{
  return image_in;
}

namespace fs = boost::filesystem;

bool IsFile(const char *path)
{
  fs::path p(path);
  if (fs::exists(p))
  {
    if (fs::is_regular_file(p))
    {
      return true;
    }
  }
  return false;
}

// TODO: remove this function and tiffio header once deskew uses ITK
unsigned short GetTIFFBitDepth(const char *path)
{
  // read bit depth from file
  unsigned short bps = UNSET_UNSIGNED_SHORT;
  TIFF *tif = TIFFOpen(path, "r");
  int defined = TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFClose(tif);
  if (!defined)
  {
    std::cerr << "GetTIFFBitDepth: no BITPERSAMPLE tag found" << std::endl;
  }
  return bps;
}
