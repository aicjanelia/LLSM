#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOBase.h>

template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ReadAndConvertImage(const char *file_path)
{
  using ImageReaderType = itk::ImageFileReader<TImageIn>;

  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(file_path);

  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject &e)
  {
    std::cerr << e.what() << std::endl;
    return nullptr;
  }

  return ConvertImage<TImageIn,TImageOut>(reader->GetOutput());
}

template <unsigned int VDimension, class TImageOut>
itk::SmartPointer<TImageOut> ReadImage(const char *file_path, const itk::IOComponentEnum component_type)
{
  switch (component_type)
  {
  default:
  case itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE:
    std::cerr << "Unknown and unsupported component type!" << std::endl;
    return nullptr;

  case itk::IOComponentEnum::UCHAR:
  {
    using PixelType = unsigned char;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::CHAR:
  {
    using PixelType = char;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::USHORT:
  {
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::SHORT:
  {
    using PixelType = short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::UINT:
  {
    using PixelType = unsigned int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::INT:
  {
    using PixelType = int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::ULONG:
  {
    using PixelType = unsigned long int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::LONG:
  {
    using PixelType = long int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::FLOAT:
  {
    using PixelType = float;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }

  case itk::IOComponentEnum::DOUBLE:
  {
    using PixelType = double;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path);
  }
  }

  return nullptr;
}

template <class TImage>
itk::SmartPointer<TImage> ReadImageFile(std::string file_path, bool verbose=false)
{
  itk::ImageIOBase::Pointer image_io = itk::ImageIOFactory::CreateImageIO(file_path.c_str(), itk::CommonEnums::IOFileMode::ReadMode);

  image_io->SetFileName(file_path.c_str());
  image_io->ReadImageInformation();

  using IOPixelType = itk::IOPixelEnum;
  const IOPixelType pixel_type = image_io->GetPixelType();

  using IOComponentType = itk::IOComponentEnum;
  const IOComponentType component_type = image_io->GetComponentType();
  
  const unsigned int image_dimension = image_io->GetNumberOfDimensions();

  if (verbose)
  {
    std::cout << "Pixel Type is " << itk::ImageIOBase::GetPixelTypeAsString(pixel_type) << std::endl;
    std::cout << "Component Type is " << image_io->GetComponentTypeAsString(component_type) << std::endl;
    std::cout << "Image Dimension is " << image_dimension << std::endl;
  }

  switch (pixel_type)
  {
  case itk::IOPixelEnum::SCALAR:
    if (image_dimension != kDimensions)
    {
      std::cerr << "Need 3-D image!" << std::endl;
      return nullptr;
    }
    return ReadImage<kDimensions, TImage>(file_path.c_str(), component_type);

  default:
    std::cerr << "not implemented yet!" << std::endl;
    return nullptr;
  }

  return nullptr;
}
