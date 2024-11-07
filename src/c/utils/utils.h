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
typename TImageOut::Pointer ConvertImage(typename TImageIn::Pointer image_in, bool scale=true)
{
  if constexpr (std::is_same<TImageIn, TImageOut>::value)
    return image_in;

  typename TImageOut::Pointer image_out = TImageOut::New();

  if (scale)
  {
    double input_min, input_max, output_min, output_max;

    GetRange<typename TImageIn::PixelType>(input_min, input_max);

    GetRange<typename TImageOut::PixelType>(output_min, output_max);

    using FilterType = itk::IntensityWindowingImageFilter<TImageIn, TImageOut>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetInput(image_in);
    filter->SetWindowMinimum(input_min);
    filter->SetWindowMaximum(input_max);
    filter->SetOutputMinimum(output_min);
    filter->SetOutputMaximum(output_max);
    filter->Update();

    image_out = filter->GetOutput();
  }
  else
  {
    using CastFilterType = itk::CastImageFilter<TImageIn, TImageOut>;
    typename CastFilterType::Pointer castFilter = CastFilterType::New();
    castFilter->SetInput(image_in);
    castFilter->Update();

    image_out = castFilter->GetOutput();
  }

  if constexpr (TImageIn::ImageDimension > 2 && TImageOut::ImageDimension > 2)
  {
    image_out->SetSpacing(image_in->GetSpacing());
  }

  return image_out;
}

template <typename TPixelType>
typename itk::Image<TPixelType, 3>::Pointer Convert2DImageTo3D(typename itk::Image<TPixelType, 2>::Pointer image2D)
{
    // Define the types
    using Image2DType = itk::Image<TPixelType, 2>;
    using Image3DType = itk::Image<TPixelType, 3>;

    // Get the size and spacing from the 2D image
    typename Image2DType::RegionType region2D = image2D->GetLargestPossibleRegion();
    typename Image2DType::SizeType size2D = region2D.GetSize();

    // Define size for the 3D image (same X and Y, Z = 1)
    typename Image3DType::SizeType size3D;
    size3D[0] = size2D[0]; // X dimension (same as 2D)
    size3D[1] = size2D[1]; // Y dimension (same as 2D)
    size3D[2] = 1;         // Z dimension (just one slice)

    // Define the region for the 3D image
    typename Image3DType::RegionType region3D;
    region3D.SetSize(size3D);

    // Create the 3D image
    typename Image3DType::Pointer image3D = Image3DType::New();
    image3D->SetRegions(region3D);
    image3D->Allocate();

    // Optionally set spacing and origin for 3D (set Z spacing to 1.0 if unknown)
    typename Image3DType::SpacingType spacing3D;
    typename Image2DType::SpacingType spacing2D = image2D->GetSpacing();
    spacing3D[0] = spacing2D[0]; // X spacing (same as 2D)
    spacing3D[1] = spacing2D[1]; // Y spacing (same as 2D)
    spacing3D[2] = 1.0;          // Z spacing (default to 1.0)
    image3D->SetSpacing(spacing3D);

    // Optionally set origin (Z origin set to 0 if unknown)
    typename Image3DType::PointType origin3D;
    typename Image2DType::PointType origin2D = image2D->GetOrigin();
    origin3D[0] = origin2D[0];   // X origin (same as 2D)
    origin3D[1] = origin2D[1];   // Y origin (same as 2D)
    origin3D[2] = 0.0;           // Z origin (default to 0.0)
    image3D->SetOrigin(origin3D);

    // Copy pixel values from the 2D image to the 3D image
    itk::ImageRegionIterator<Image2DType> it2D(image2D, region2D);
    typename Image3DType::IndexType index3D;
    
    for (it2D.GoToBegin(); !it2D.IsAtEnd(); ++it2D)
    {
        typename Image2DType::IndexType index2D = it2D.GetIndex();

        // Set the 3D index (same X, Y as 2D, Z=0 for the slice)
        index3D[0] = index2D[0]; // X index
        index3D[1] = index2D[1]; // Y index
        index3D[2] = 0;          // Z index (single slice)

        // Copy the pixel value
        image3D->SetPixel(index3D, it2D.Get());
    }

    return image3D;
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
