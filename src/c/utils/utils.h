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

template <class TImage>
void GetRange(itk::SmartPointer<TImage> image, double &min_val, double &max_val)
{
  min_val = 0.0;
  max_val = 1.0;
}

void GetRange(itk::SmartPointer<itk::Image<unsigned char, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<unsigned char>::min());
  max_val = double(std::numeric_limits<unsigned char>::max());
}

void GetRange(itk::SmartPointer<itk::Image<char, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<char>::min());
  max_val = double(std::numeric_limits<char>::max());
}

void GetRange(itk::SmartPointer<itk::Image<unsigned short, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<unsigned short>::min());
  max_val = double(std::numeric_limits<unsigned short>::max());
}

void GetRange(itk::SmartPointer<itk::Image<short, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<short>::min());
  max_val = double(std::numeric_limits<short>::max());
}

void GetRange(itk::SmartPointer<itk::Image<unsigned int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<unsigned int>::min());
  max_val = double(std::numeric_limits<unsigned int>::max());
}

void GetRange(itk::SmartPointer<itk::Image<int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<int>::min());
  max_val = double(std::numeric_limits<int>::max());
}

void GetRange(itk::SmartPointer<itk::Image<unsigned long int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<unsigned long int>::min());
  max_val = double(std::numeric_limits<unsigned long int>::max());
}

void GetRange(itk::SmartPointer<itk::Image<long int, kDimensions>> image, double &min_val, double &max_val)
{
  min_val = double(std::numeric_limits<long int>::min());
  max_val = double(std::numeric_limits<long int>::max());
}

template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ConvertImage(itk::SmartPointer<TImageIn> image_in)
{
  if constexpr (std::is_same<TImageIn, TImageOut>::value)
    return image_in;

  double input_min, input_max, output_min, output_max;

  GetRange(image_in, input_min, input_max);
  itk::SmartPointer<TImageOut> image_out = TImageOut::New();
  GetRange(image_out, output_min, output_max);

  using FilterType = itk::IntensityWindowingImageFilter<TImageIn, TImageOut>;
  typename FilterType::Pointer filter = FilterType::New();
  filter->SetInput(image_in);
  filter->SetWindowMinimum(input_min);
  filter->SetWindowMaximum(input_max);
  filter->SetOutputMinimum(output_min);
  filter->SetOutputMaximum(output_max);
  filter->Update();

  return filter->GetOutput();
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
