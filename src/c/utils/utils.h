#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>
#include <limits>

#include <itkImage.h>
#include <itkCastImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

namespace fs = boost::filesystem;

template <class T>
void GetRange(double &min_val, double &max_val)
{
  if (std::is_same<T, unsigned char>::value)
  {
    min_val = double(std::numeric_limits<unsigned char>::min());
    max_val = double(std::numeric_limits<unsigned char>::max());
  }
  else if (std::is_same<T, char>::value)
  {
    min_val = double(std::numeric_limits<char>::min());
    max_val = double(std::numeric_limits<char>::max());
  }
  else if (std::is_same<T, unsigned short>::value)
  {
    min_val = double(std::numeric_limits<unsigned short>::min());
    max_val = double(std::numeric_limits<unsigned short>::max());
  }
  else if (std::is_same<T, short>::value)
  {
    min_val = double(std::numeric_limits<short>::min());
    max_val = double(std::numeric_limits<short>::max());
  }
  else if (std::is_same<T, unsigned int>::value)
  {
    min_val = double(std::numeric_limits<unsigned int>::min());
    max_val = double(std::numeric_limits<unsigned int>::max());
  }
  else if (std::is_same<T, int>::value)
  {
    min_val = double(std::numeric_limits<int>::min());
    max_val = double(std::numeric_limits<int>::max());
  }
  else if (std::is_same<T, unsigned long int>::value)
  {
    min_val = double(std::numeric_limits<unsigned long int>::min());
    max_val = double(std::numeric_limits<unsigned long int>::max());
  }
  else if (std::is_same<T, long int>::value)
  {
    min_val = double(std::numeric_limits<long int>::min());
    max_val = double(std::numeric_limits<long int>::max());
  }
  else
  {
    min_val = 0.0;
    max_val = 1.0;
  }
}

template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ConvertImage(itk::SmartPointer<TImageIn> image_in)
{
  if constexpr (std::is_same<TImageIn, TImageOut>::value)
    return image_in;

  double input_min, input_max, output_min, output_max;

  GetRange<TImageIn::PixelType>(input_min, input_max);
  typename itk::SmartPointer<TImageOut> image_out = TImageOut::New();
  GetRange<TImageOut::PixelType>(output_min, output_max);

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

std::string AppendPath(std::string path, std::string label)
{
  fs::path in_path(path);
  fs::path out_path(in_path.parent_path());
  out_path /= in_path.stem();
  out_path += label;
  out_path += in_path.extension();
  return out_path.string(); 
}
