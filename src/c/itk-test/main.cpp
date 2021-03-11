#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageIOBase.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkCastImageFilter.h"
#include <limits>

#include "itkImageFileWriter.h"

template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ReadImage(const char *fileName)
{
  using ImageReaderType = itk::ImageFileReader<TImageIn>;

  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(fileName);

  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject &e)
  {
    std::cerr << e.what() << std::endl;
    return itk::SmartPointer<TImageOut>();
  }

  using RescaleType = itk::RescaleIntensityImageFilter<TImageIn, TImageOut>;
  typename RescaleType::Pointer rescale = RescaleType::New();
  rescale->SetInput(reader->GetOutput());
  rescale->SetOutputMinimum(0);
  rescale->SetOutputMaximum(1.0);
  rescale->Update();

  // using FilterType = itk::CastImageFilter<TImageIn, TImageOut>;
  // typename FilterType::Pointer filter = FilterType::New();
  // filter->SetInput(rescale->GetOutput());
  // filter->Update();

  return rescale->GetOutput();
}

template <unsigned int VDimension, class TImageOut>
itk::SmartPointer<TImageOut> ReadImage(const char *inputFileName, const itk::IOComponentEnum componentType)
{
  switch (componentType)
  {
  default:
  case itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE:
    std::cerr << "Unknown and unsupported component type!" << std::endl;
    return itk::SmartPointer<TImageOut>();

  case itk::IOComponentEnum::UCHAR:
  {
    using PixelType = unsigned char;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::CHAR:
  {
    using PixelType = char;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::USHORT:
  {
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::SHORT:
  {
    using PixelType = short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::UINT:
  {
    using PixelType = unsigned int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::INT:
  {
    using PixelType = int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::ULONG:
  {
    using PixelType = unsigned long;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::LONG:
  {
    using PixelType = long;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::FLOAT:
  {
    using PixelType = float;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }

  case itk::IOComponentEnum::DOUBLE:
  {
    using PixelType = double;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadImage<ImageType, TImageOut>(inputFileName);
  }
  }

  return itk::SmartPointer<TImageOut>();
}

int main(int argc, char *argv[])
{
  std::string inputFileName("/nrs/aic/instruments/llsm/pipeline-test/no-deskew/2020-03-11/4-TL10min/642-4ms-20p_488-10ms-20p_25um_CamA_ch1_CAM1_stack0000_488nm_0000000msec_0023277664msecAbs_000x_000y_000z_0000t.tif");

  itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(inputFileName.c_str(), itk::CommonEnums::IOFileMode::ReadMode);

  imageIO->SetFileName(inputFileName.c_str());
  imageIO->ReadImageInformation();

  using IOPixelType = itk::IOPixelEnum;
  const IOPixelType pixelType = imageIO->GetPixelType();

  std::cout << "Pixel Type is " << itk::ImageIOBase::GetPixelTypeAsString(pixelType) << std::endl;

  using IOComponentType = itk::IOComponentEnum;
  const IOComponentType componentType = imageIO->GetComponentType();

  std::cout << "Component Type is " << imageIO->GetComponentTypeAsString(componentType) << std::endl;

  const unsigned int imageDimension = imageIO->GetNumberOfDimensions();

  std::cout << "Image Dimension is " << imageDimension << std::endl;

  using PixelType = float;
  constexpr unsigned int dims = 3;
  using ImageType = itk::Image<PixelType,dims>;
  typename itk::SmartPointer<ImageType> image;

  switch (pixelType)
  {
  case itk::IOPixelEnum::SCALAR:
  {
    if (imageDimension == 2)
    {
      // return ReadScalarImage<2>(inputFileName.c_str(), componentType);
      std::cerr << "Need 3-D image!" << std::endl;
      return EXIT_FAILURE;
    }
    else if (imageDimension == dims)
    {
      image = ReadImage<dims,ImageType>(inputFileName.c_str(), componentType);
    }
    else if (imageDimension == 4)
    {
      // return ReadScalarImage<4>(inputFileName.c_str(), componentType);
      std::cerr << "Need 3-D image!" << std::endl;
      return EXIT_FAILURE;
    }
    break;
  }
  default:
    std::cerr << "not implemented yet!" << std::endl;
    return EXIT_FAILURE;
  }

  using WriterType = itk::ImageFileWriter<ImageType>;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName("test.tiff");
  writer->SetInput(image);
  writer->Update();

  return EXIT_SUCCESS;
}