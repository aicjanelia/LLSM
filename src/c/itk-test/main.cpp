#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageIOBase.h"

template <class TImage>
int ReadImage(const char *fileName, typename TImage::Pointer image)
{
  using ImageType = TImage;
  using ImageReaderType = itk::ImageFileReader<ImageType>;

  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(fileName);

  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  image->Graft(reader->GetOutput());

  return EXIT_SUCCESS;
}

template <unsigned int VDimension>
int ReadScalarImage(const char *inputFileName, const itk::IOComponentEnum componentType)
{
  switch (componentType)
  {
  default:
  case itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE:
    std::cerr << "Unknown and unsupported component type!" << std::endl;
    return EXIT_FAILURE;

  case itk::IOComponentEnum::UCHAR:
  {
    using PixelType = unsigned char;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::CHAR:
  {
    using PixelType = char;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::USHORT:
  {
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::SHORT:
  {
    using PixelType = short;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::UINT:
  {
    using PixelType = unsigned int;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::INT:
  {
    using PixelType = int;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::ULONG:
  {
    using PixelType = unsigned long;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::LONG:
  {
    using PixelType = long;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::FLOAT:
  {
    using PixelType = float;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }

  case itk::IOComponentEnum::DOUBLE:
  {
    using PixelType = double;
    using ImageType = itk::Image<PixelType, VDimension>;

    typename ImageType::Pointer image = ImageType::New();

    if (ReadImage<ImageType>(inputFileName, image) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }

    std::cout << image << std::endl;
    break;
  }
  }
  return EXIT_SUCCESS;
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

  switch (pixelType)
  {
  case itk::IOPixelEnum::SCALAR:
  {
    if (imageDimension == 2)
    {
      return ReadScalarImage<2>(inputFileName.c_str(), componentType);
    }
    else if (imageDimension == 3)
    {
      return ReadScalarImage<3>(inputFileName.c_str(), componentType);
    }
    else if (imageDimension == 4)
    {
      return ReadScalarImage<4>(inputFileName.c_str(), componentType);
    }
  }

  default:
    std::cerr << "not implemented yet!" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}