#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOBase.h>
#include "itk_tiff.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

class TiffStackBlockReader
{
public:
  explicit TiffStackBlockReader(const std::string &file_path)
      : file_path_(file_path), tiff_(TIFFOpen(file_path.c_str(), "r"))
  {
    spacing_.Fill(1.0);
    if (tiff_ == nullptr)
    {
      throw std::runtime_error("Unable to open TIFF input: " + file_path);
    }

    try
    {
      const tdir_t directory_count = TIFFNumberOfDirectories(tiff_);
      if (directory_count == 0)
      {
        throw std::runtime_error("TIFF input contains no image directories: " + file_path);
      }
      depth_ = static_cast<std::uint64_t>(directory_count);

      if (!TIFFSetDirectory(tiff_, 0))
      {
        throw std::runtime_error("Unable to read the first TIFF directory: " + file_path);
      }
      ReadAndValidateDirectoryInfo(true);
    }
    catch (...)
    {
      TIFFClose(tiff_);
      tiff_ = nullptr;
      throw;
    }
  }

  ~TiffStackBlockReader()
  {
    if (tiff_ != nullptr)
    {
      TIFFClose(tiff_);
    }
  }

  TiffStackBlockReader(const TiffStackBlockReader &) = delete;
  TiffStackBlockReader &operator=(const TiffStackBlockReader &) = delete;

  std::uint64_t GetWidth() const { return width_; }
  std::uint64_t GetHeight() const { return height_; }
  std::uint64_t GetDepth() const { return depth_; }
  std::uint16_t GetBitsPerSample() const { return bits_per_sample_; }
  std::uint16_t GetSampleFormat() const { return sample_format_; }
  const kImageType::SpacingType &GetSpacing() const { return spacing_; }

  kImageType::Pointer ReadBlock(std::uint64_t first_slice,
                                std::uint64_t slice_count,
                                const kImageType::SpacingType &spacing,
                                kPixelType subtract_constant=0.0)
  {
    if (slice_count == 0 || first_slice >= depth_ || slice_count > depth_ - first_slice)
    {
      throw std::out_of_range("Requested TIFF Z block is outside the input stack");
    }
    if (first_slice > static_cast<std::uint64_t>(std::numeric_limits<tdir_t>::max()) ||
        first_slice + slice_count - 1 >
            static_cast<std::uint64_t>(std::numeric_limits<tdir_t>::max()))
    {
      throw std::runtime_error("TIFF directory index exceeds the libtiff limit");
    }

    kImageType::IndexType index;
    index.Fill(0);
    kImageType::SizeType size;
    size[0] = width_;
    size[1] = height_;
    size[2] = slice_count;
    kImageType::RegionType region(index, size);

    kImageType::Pointer image = kImageType::New();
    image->SetRegions(region);
    image->SetSpacing(spacing);
    image->Allocate();

    kPixelType *output = image->GetBufferPointer();
    for (std::uint64_t local_slice = 0; local_slice < slice_count; ++local_slice)
    {
      const tdir_t directory = static_cast<tdir_t>(first_slice + local_slice);
      if (!TIFFSetDirectory(tiff_, directory))
      {
        std::ostringstream message;
        message << "Unable to select TIFF directory " << directory;
        throw std::runtime_error(message.str());
      }
      ReadAndValidateDirectoryInfo(false);
      ReadDirectory(output, local_slice, subtract_constant);
    }
    return image;
  }

private:
  void ReadAndValidateDirectoryInfo(bool first_directory)
  {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint16_t bits_per_sample = 0;
    std::uint16_t samples_per_pixel = 0;
    std::uint16_t sample_format = SAMPLEFORMAT_UINT;
    std::uint16_t planar_configuration = PLANARCONFIG_CONTIG;
    std::uint16_t photometric = PHOTOMETRIC_MINISBLACK;
    std::uint16_t orientation = ORIENTATION_TOPLEFT;

    if (!TIFFGetField(tiff_, TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(tiff_, TIFFTAG_IMAGELENGTH, &height))
    {
      throw std::runtime_error("TIFF directory is missing width or height");
    }
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_SAMPLEFORMAT, &sample_format);
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_PLANARCONFIG, &planar_configuration);
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_PHOTOMETRIC, &photometric);
    TIFFGetFieldDefaulted(tiff_, TIFFTAG_ORIENTATION, &orientation);

    if (width == 0 || height == 0)
    {
      throw std::runtime_error("TIFF directory has an empty image plane");
    }
    if (samples_per_pixel != 1 || planar_configuration != PLANARCONFIG_CONTIG)
    {
      throw std::runtime_error("Only single-channel contiguous TIFF stacks are supported");
    }
    if (photometric != PHOTOMETRIC_MINISBLACK && photometric != PHOTOMETRIC_MINISWHITE)
    {
      throw std::runtime_error("Only grayscale TIFF stacks are supported");
    }
    if (orientation != ORIENTATION_TOPLEFT && orientation != ORIENTATION_BOTLEFT)
    {
      throw std::runtime_error("Only top-left and bottom-left TIFF orientations are supported");
    }
    if (TIFFIsTiled(tiff_))
    {
      throw std::runtime_error("Tiled TIFF input is not supported by the Z-block reader");
    }

    if (first_directory)
    {
      width_ = width;
      height_ = height;
      bits_per_sample_ = bits_per_sample;
      sample_format_ = sample_format;
      photometric_ = photometric;
      orientation_ = orientation;

      float x_resolution = 0.0f;
      float y_resolution = 0.0f;
      if (TIFFGetField(tiff_, TIFFTAG_XRESOLUTION, &x_resolution) && x_resolution > 0.0f)
      {
        spacing_[0] = 1.0 / x_resolution;
      }
      if (TIFFGetField(tiff_, TIFFTAG_YRESOLUTION, &y_resolution) && y_resolution > 0.0f)
      {
        spacing_[1] = 1.0 / y_resolution;
      }

      char *description = nullptr;
      if (TIFFGetField(tiff_, TIFFTAG_IMAGEDESCRIPTION, &description) && description != nullptr)
      {
        const char *spacing_text = std::strstr(description, "spacing=");
        if (spacing_text != nullptr)
        {
          char *end = nullptr;
          const double z_spacing = std::strtod(spacing_text + 8, &end);
          if (end != spacing_text + 8 && std::isfinite(z_spacing) && z_spacing > 0.0)
          {
            spacing_[2] = z_spacing;
          }
        }
      }
    }
    else if (width_ != width || height_ != height ||
             bits_per_sample_ != bits_per_sample || sample_format_ != sample_format ||
             photometric_ != photometric || orientation_ != orientation)
    {
      throw std::runtime_error("All TIFF directories must use identical dimensions and pixel types");
    }
  }

  template <typename TPixel>
  static double NormalizeInteger(TPixel value)
  {
    const long double minimum =
        static_cast<long double>(std::numeric_limits<TPixel>::lowest());
    const long double maximum =
        static_cast<long double>(std::numeric_limits<TPixel>::max());
    return static_cast<double>((static_cast<long double>(value) - minimum) /
                               (maximum - minimum));
  }

  static double ClampUnit(double value)
  {
    if (!std::isfinite(value))
    {
      return 0.0;
    }
    return std::max(0.0, std::min(1.0, value));
  }

  template <typename TPixel>
  void ReadIntegerDirectory(kPixelType *output,
                            std::uint64_t local_slice,
                            kPixelType subtract_constant)
  {
    ReadTypedDirectory<TPixel>(output, local_slice, subtract_constant,
                               [](TPixel value) { return NormalizeInteger(value); });
  }

  template <typename TPixel>
  void ReadFloatingDirectory(kPixelType *output,
                             std::uint64_t local_slice,
                             kPixelType subtract_constant)
  {
    ReadTypedDirectory<TPixel>(output, local_slice, subtract_constant,
                               [](TPixel value) { return ClampUnit(static_cast<double>(value)); });
  }

  template <typename TPixel, typename TNormalizer>
  void ReadTypedDirectory(kPixelType *output,
                          std::uint64_t local_slice,
                          kPixelType subtract_constant,
                          TNormalizer normalize)
  {
    const tmsize_t scanline_size = TIFFScanlineSize(tiff_);
    const std::uint64_t required_size = width_ * sizeof(TPixel);
    if (scanline_size < 0 || static_cast<std::uint64_t>(scanline_size) < required_size)
    {
      throw std::runtime_error("TIFF scanline is smaller than the image width");
    }
    std::vector<unsigned char> scanline(static_cast<std::size_t>(scanline_size));
    const std::uint64_t plane_voxels = width_ * height_;

    for (std::uint32_t row = 0; row < height_; ++row)
    {
      if (TIFFReadScanline(tiff_, scanline.data(), row, 0) < 0)
      {
        std::ostringstream message;
        message << "Unable to read TIFF scanline " << row;
        throw std::runtime_error(message.str());
      }
      const std::uint64_t output_row = orientation_ == ORIENTATION_BOTLEFT
                                           ? height_ - row - 1
                                           : row;
      kPixelType *destination = output + local_slice * plane_voxels + output_row * width_;
      for (std::uint64_t column = 0; column < width_; ++column)
      {
        TPixel value;
        std::memcpy(&value, scanline.data() + column * sizeof(TPixel), sizeof(TPixel));
        double normalized = normalize(value);
        if (photometric_ == PHOTOMETRIC_MINISWHITE)
        {
          normalized = 1.0 - normalized;
        }
        destination[column] = ClampUnit(normalized - subtract_constant);
      }
    }
  }

  void ReadDirectory(kPixelType *output,
                     std::uint64_t local_slice,
                     kPixelType subtract_constant)
  {
    if (sample_format_ == SAMPLEFORMAT_IEEEFP)
    {
      if (bits_per_sample_ == 32)
      {
        ReadFloatingDirectory<float>(output, local_slice, subtract_constant);
        return;
      }
      if (bits_per_sample_ == 64)
      {
        ReadFloatingDirectory<double>(output, local_slice, subtract_constant);
        return;
      }
    }
    else if (sample_format_ == SAMPLEFORMAT_INT)
    {
      switch (bits_per_sample_)
      {
      case 8: ReadIntegerDirectory<std::int8_t>(output, local_slice, subtract_constant); return;
      case 16: ReadIntegerDirectory<std::int16_t>(output, local_slice, subtract_constant); return;
      case 32: ReadIntegerDirectory<std::int32_t>(output, local_slice, subtract_constant); return;
      case 64: ReadIntegerDirectory<std::int64_t>(output, local_slice, subtract_constant); return;
      default: break;
      }
    }
    else if (sample_format_ == SAMPLEFORMAT_UINT || sample_format_ == SAMPLEFORMAT_VOID)
    {
      switch (bits_per_sample_)
      {
      case 8: ReadIntegerDirectory<std::uint8_t>(output, local_slice, subtract_constant); return;
      case 16: ReadIntegerDirectory<std::uint16_t>(output, local_slice, subtract_constant); return;
      case 32: ReadIntegerDirectory<std::uint32_t>(output, local_slice, subtract_constant); return;
      case 64: ReadIntegerDirectory<std::uint64_t>(output, local_slice, subtract_constant); return;
      default: break;
      }
    }

    std::ostringstream message;
    message << "Unsupported TIFF sample format " << sample_format_
            << " with " << bits_per_sample_ << " bits per sample";
    throw std::runtime_error(message.str());
  }

  std::string file_path_;
  TIFF *tiff_ = nullptr;
  std::uint64_t width_ = 0;
  std::uint64_t height_ = 0;
  std::uint64_t depth_ = 0;
  std::uint16_t bits_per_sample_ = 0;
  std::uint16_t sample_format_ = SAMPLEFORMAT_UINT;
  std::uint16_t photometric_ = PHOTOMETRIC_MINISBLACK;
  std::uint16_t orientation_ = ORIENTATION_TOPLEFT;
  kImageType::SpacingType spacing_;
};

template <class TImageIn, class TImageOut>
typename TImageOut::Pointer ReadAndConvertImage(const char *file_path, bool scale=true)
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

  return ConvertImage<TImageIn,TImageOut>(reader->GetOutput(), scale);
}

template <unsigned int VDimension, class TImageOut>
typename TImageOut::Pointer ReadImage(const char *file_path, const itk::IOComponentEnum component_type, bool scale=true)
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

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::CHAR:
  {
    using PixelType = char;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::USHORT:
  {
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::SHORT:
  {
    using PixelType = short;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::UINT:
  {
    using PixelType = unsigned int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::INT:
  {
    using PixelType = int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::ULONG:
  {
    using PixelType = unsigned long int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::LONG:
  {
    using PixelType = long int;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::FLOAT:
  {
    using PixelType = float;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }

  case itk::IOComponentEnum::DOUBLE:
  {
    using PixelType = double;
    using ImageType = itk::Image<PixelType, VDimension>;

    return ReadAndConvertImage<ImageType, TImageOut>(file_path, scale);
  }
  }

  return nullptr;
}

template <class TImage>
itk::SmartPointer<TImage> ReadImageFile(std::string file_path, bool verbose=false, bool scale=true)
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
    /*
    if (image_dimension != kDimensions)
    {
      std::cerr << "Need 3-D image!" << std::endl;
      return nullptr;
    }
    */
    return ReadImage<kDimensions, TImage>(file_path.c_str(), component_type, scale);

  default:
    std::cerr << "not implemented yet!" << std::endl;
    return nullptr;
  }

  return nullptr;
}
