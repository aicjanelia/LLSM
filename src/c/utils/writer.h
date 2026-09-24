#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
#include "itkMetaDataDictionary.h"
#include "itkTIFFImageIO.h"

#include "itksys/SystemTools.hxx"
#include "itkMetaDataObject.h"

#include "itkMacro.h"

#include "itk_tiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

// libtiff sample format for the pixel type being written. Without this tag readers
// default to SAMPLEFORMAT_UINT, which makes 32-bit float output (--bit-depth 32) be
// misinterpreted as 32-bit unsigned integer (e.g. by ImageJ's TiffDecoder).
template <typename TPixel>
constexpr int TiffSampleFormat()
{
    return std::is_floating_point<TPixel>::value
               ? SAMPLEFORMAT_IEEEFP
               : (std::is_signed<TPixel>::value ? SAMPLEFORMAT_INT : SAMPLEFORMAT_UINT);
}

class TiffStackBlockWriter
{
public:
    TiffStackBlockWriter(const std::string &file_path,
                         std::uint64_t width,
                         std::uint64_t height,
                         std::uint64_t depth,
                         unsigned int bit_depth,
                         const kImageType::SpacingType &spacing)
        : file_path_(file_path), width_(width), height_(height), depth_(depth),
          bit_depth_(bit_depth), spacing_(spacing)
    {
        if (width_ == 0 || height_ == 0 || depth_ == 0)
        {
            throw std::invalid_argument("TIFF output dimensions must be greater than zero");
        }
        if (width_ > std::numeric_limits<std::uint32_t>::max() ||
            height_ > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::runtime_error("TIFF output width or height exceeds uint32_t");
        }
        if (bit_depth_ != 8 && bit_depth_ != 16 && bit_depth_ != 32)
        {
            throw std::invalid_argument("TIFF output bit depth must be 8, 16, or 32");
        }

        const std::uint64_t bytes_per_pixel = bit_depth_ / 8;
        if (width_ > std::numeric_limits<std::uint32_t>::max() / bytes_per_pixel)
        {
            throw std::runtime_error("TIFF output scanline exceeds the libtiff size limit");
        }
        const long double estimated_size = static_cast<long double>(width_) *
                                           static_cast<long double>(height_) *
                                           static_cast<long double>(depth_) *
                                           static_cast<long double>(bytes_per_pixel);
        const long double big_tiff_threshold =
            3.0L * 1024.0L * 1024.0L * 1024.0L;
        const char *mode = estimated_size >= big_tiff_threshold ? "w8" : "w";
        // Stage beside the destination so Finish() can publish the complete
        // stack with a same-filesystem rename. A failed run must not replace
        // an existing result or leave a truncated TIFF at the final path.
        const fs::path destination(file_path_);
        do
        {
            staging_directory_ = destination.parent_path() /
                fs::unique_path(".decon-partial-%%%%-%%%%-%%%%-%%%%");
        }
        while (!fs::create_directory(staging_directory_));
        staging_path_ = staging_directory_ / "stack.tif";
        tiff_ = TIFFOpen(staging_path_.string().c_str(), mode);
        if (tiff_ == nullptr)
        {
            RemoveStagingFile();
            throw std::runtime_error("Failed to open TIFF file for block writing: " + file_path_);
        }
    }

    ~TiffStackBlockWriter()
    {
        if (tiff_ != nullptr)
        {
            TIFFClose(tiff_);
        }
        RemoveStagingFile();
    }

    TiffStackBlockWriter(const TiffStackBlockWriter &) = delete;
    TiffStackBlockWriter &operator=(const TiffStackBlockWriter &) = delete;

    void AppendCore(kImageType::Pointer image,
                    std::uint64_t first_local_slice,
                    std::uint64_t slice_count)
    {
        if (tiff_ == nullptr)
        {
            throw std::logic_error("Cannot append to a closed TIFF output");
        }
        if (image == nullptr)
        {
            throw std::invalid_argument("Cannot write a null image block");
        }
        const kImageType::RegionType region = image->GetLargestPossibleRegion();
        const kImageType::SizeType size = region.GetSize();
        if (size[0] != width_ || size[1] != height_ ||
            first_local_slice > size[2] || slice_count > size[2] - first_local_slice)
        {
            throw std::out_of_range("Output core is outside the deconvolved image block");
        }
        if (slices_written_ > depth_ || slice_count > depth_ - slices_written_)
        {
            throw std::out_of_range("Output core exceeds the declared TIFF stack depth");
        }

        const kPixelType *buffer = image->GetBufferPointer();
        const std::uint64_t plane_voxels = width_ * height_;
        for (std::uint64_t local_slice = first_local_slice;
             local_slice < first_local_slice + slice_count;
             ++local_slice)
        {
            SetDirectoryTags();
            const kPixelType *source = buffer + local_slice * plane_voxels;
            if (bit_depth_ == 8)
            {
                WriteSlice<std::uint8_t>(source);
            }
            else if (bit_depth_ == 16)
            {
                WriteSlice<std::uint16_t>(source);
            }
            else
            {
                WriteSlice<float>(source);
            }

            ++slices_written_;
            if (slices_written_ < depth_ && TIFFWriteDirectory(tiff_) == 0)
            {
                CloseAfterError();
                throw std::runtime_error("Failed to write TIFF directory for output slice");
            }
        }
    }

    void Finish()
    {
        if (finished_)
        {
            return;
        }
        if (slices_written_ != depth_)
        {
            std::ostringstream message;
            message << "TIFF output has " << slices_written_ << " slices; expected " << depth_;
            throw std::runtime_error(message.str());
        }
        if (tiff_ == nullptr)
        {
            throw std::runtime_error("TIFF output was closed before completion: " + file_path_);
        }
        if (TIFFFlush(tiff_) == 0)
        {
            CloseAfterError();
            throw std::runtime_error("Failed to flush TIFF output: " + file_path_);
        }
        TIFFClose(tiff_);
        tiff_ = nullptr;
        fs::rename(staging_path_, fs::path(file_path_));
        finished_ = true;
        RemoveStagingFile();
    }

private:
    void RemoveStagingFile() noexcept
    {
        boost::system::error_code error;
        if (!staging_path_.empty())
            fs::remove(staging_path_, error);
        if (!staging_directory_.empty())
            fs::remove(staging_directory_, error);
    }

    void SetDirectoryTags()
    {
        TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, static_cast<std::uint32_t>(width_));
        TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH, static_cast<std::uint32_t>(height_));
        TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, static_cast<int>(bit_depth_));
        TIFFSetField(tiff_, TIFFTAG_SAMPLEFORMAT,
                     bit_depth_ == 32 ? SAMPLEFORMAT_IEEEFP : SAMPLEFORMAT_UINT);
        TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tiff_, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP,
                     TIFFDefaultStripSize(tiff_,
                                          static_cast<std::uint32_t>(width_ * (bit_depth_ / 8))));
        TIFFSetField(tiff_, TIFFTAG_XRESOLUTION, 1.0 / spacing_[0]);
        TIFFSetField(tiff_, TIFFTAG_YRESOLUTION, 1.0 / spacing_[1]);
        TIFFSetField(tiff_, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        if (slices_written_ == 0)
        {
            char description[512];
            snprintf(description, sizeof(description),
                     "ImageJ=1.53\nimages=%llu\nslices=%llu\nspacing=%.6f\nunit=pixel\nhyperstack=false\nmode=grayscale\nloop=false",
                     static_cast<unsigned long long>(depth_),
                     static_cast<unsigned long long>(depth_), spacing_[2]);
            TIFFSetField(tiff_, TIFFTAG_IMAGEDESCRIPTION, description);
        }
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
    static TPixel ConvertOutputImpl(kPixelType value, std::true_type)
    {
        return static_cast<TPixel>(ClampUnit(value));
    }

    template <typename TPixel>
    static TPixel ConvertOutputImpl(kPixelType value, std::false_type)
    {
        return static_cast<TPixel>(ClampUnit(value) *
                                   static_cast<double>(std::numeric_limits<TPixel>::max()));
    }

    template <typename TPixel>
    static TPixel ConvertOutput(kPixelType value)
    {
        return ConvertOutputImpl<TPixel>(value, std::is_floating_point<TPixel>{});
    }

    template <typename TPixel>
    void WriteSlice(const kPixelType *source)
    {
        std::vector<TPixel> row_buffer(static_cast<std::size_t>(width_));
        for (std::uint32_t row = 0; row < height_; ++row)
        {
            const kPixelType *source_row = source + static_cast<std::uint64_t>(row) * width_;
            for (std::uint64_t column = 0; column < width_; ++column)
            {
                row_buffer[static_cast<std::size_t>(column)] = ConvertOutput<TPixel>(source_row[column]);
            }
            if (TIFFWriteScanline(tiff_, row_buffer.data(), row, 0) < 0)
            {
                CloseAfterError();
                throw std::runtime_error("Failed to write TIFF scanline for output slice");
            }
        }
    }

    void CloseAfterError()
    {
        if (tiff_ != nullptr)
        {
            TIFFClose(tiff_);
            tiff_ = nullptr;
        }
    }

    std::string file_path_;
    fs::path staging_directory_;
    fs::path staging_path_;
    bool finished_ = false;
    TIFF *tiff_ = nullptr;
    std::uint64_t width_ = 0;
    std::uint64_t height_ = 0;
    std::uint64_t depth_ = 0;
    unsigned int bit_depth_ = 0;
    kImageType::SpacingType spacing_;
    std::uint64_t slices_written_ = 0;
};


template <typename TPixel, unsigned int VDimension>
void SaveImageAsTiff(typename itk::Image<TPixel, VDimension>::Pointer itkImage, const std::string& filename) {

    // Ensure the image is 2D
    if (VDimension != 2) {
        throw std::runtime_error("This function supports only 2D images.");
    }

    using ImageType = itk::Image<TPixel, VDimension>;
    typename ImageType::RegionType region = itkImage->GetLargestPossibleRegion();
    typename ImageType::SizeType size = region.GetSize();
    typename ImageType::SpacingType spacing = itkImage->GetSpacing();

    size_t width = size[0];
    size_t height = size[1];

    std::vector<TPixel> buffer(width * height);

    itk::ImageRegionConstIterator<ImageType> it(itkImage, region);
    size_t index = 0;
    for (it.GoToBegin(); !it.IsAtEnd(); ++it, ++index) {
        buffer[index] = it.Get();
    }

    // Calculate estimated file size
    size_t estimated_size = width * height * sizeof(TPixel);
    const size_t size_threshold = static_cast<size_t>(3.0 * 1024 * 1024 * 1024); // 3.0GB

    // Write a classic TIFF ("w") rather than BigTIFF ("w8") so that ImageJ opens the file
    // with its fast native reader instead of the Bio-Formats importer, which is very slow
    // when browsing many MIPs. A 2D projection is orders of magnitude below the 4GB classic
    // TIFF limit; only fall back to BigTIFF if a single plane would not fit.
    const char* mode = (estimated_size >= size_threshold) ? "w8" : "w";
    TIFF* tiff = TIFFOpen(filename.c_str(), mode);
    if (!tiff) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    // Set TIFF fields
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(width));
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(height));
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1); // Grayscale image
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, static_cast<int>(sizeof(TPixel) * 8));
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, TiffSampleFormat<TPixel>());
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, width * sizeof(TPixel)));

    // TIFF resolution is pixels per unit, which is the inverse of spacing (unit per pixel)
    TIFFSetField(tiff, TIFFTAG_XRESOLUTION, 1.0 / spacing[0]);
    TIFFSetField(tiff, TIFFTAG_YRESOLUTION, 1.0 / spacing[1]);
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    // Mark the file as ImageJ format, mirroring the 3D writer. A single plane needs no
    // stack fields; ImageJ's TiffDecoder infers nImages=1 from the single IFD.
    char description[128];
    snprintf(description, sizeof(description),
             "ImageJ=1.53\nimages=1\nslices=1\nunit=pixel\nmode=grayscale");
    TIFFSetField(tiff, TIFFTAG_IMAGEDESCRIPTION, description);

    for (size_t row = 0; row < height; ++row) {
        if (TIFFWriteScanline(tiff, buffer.data() + row * width, row, 0) < 0) {
            TIFFClose(tiff);
            throw std::runtime_error("Failed to write TIFF scanline.");
        }
    }

    TIFFClose(tiff);
}

template <typename TPixel, unsigned int VDimension>
void Save3DImageAsTiffStackWithResolutions(typename itk::Image<TPixel, VDimension>::Pointer itkImage, const std::string& filename) {

  // Ensure the image is 3D
    if (VDimension != 3) {
        throw std::runtime_error("This function supports only 3D images.");
    }
    
    using ImageType = itk::Image<TPixel, VDimension>;
    typename ImageType::RegionType region = itkImage->GetLargestPossibleRegion();
    typename ImageType::IndexType startIndex = region.GetIndex();
    typename ImageType::SizeType size = region.GetSize();
    typename ImageType::SpacingType spacing = itkImage->GetSpacing();

    size_t width = size[0];
    size_t height = size[1];
    size_t depth = size[2]; 

    // Calculate estimated file size
    size_t estimated_size = width * height * depth * sizeof(TPixel);
    const size_t size_threshold = static_cast<size_t>(3.0 * 1024 * 1024 * 1024); // 3.0GB

    // Use BigTIFF format ("w8") for files larger than 3.0GB, otherwise use standard mode ("w")
    const char* mode = (estimated_size >= size_threshold) ? "w8" : "w";
    TIFF* tiff = TIFFOpen(filename.c_str(), mode);
    if (!tiff) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    for (size_t slice = 0; slice < depth; ++slice) {
        std::vector<TPixel> buffer(width * height);
        typename ImageType::IndexType start = { {startIndex[0], startIndex[1],
            startIndex[2] + static_cast<typename ImageType::IndexValueType>(slice)} };
        typename ImageType::SizeType sliceSize = { {width, height, 1} };
        typename ImageType::RegionType sliceRegion(start, sliceSize);

        itk::ImageRegionConstIterator<ImageType> it(itkImage, sliceRegion);
        size_t index = 0;
        for (it.GoToBegin(); !it.IsAtEnd(); ++it, ++index) {
            buffer[index] = it.Get();
        }

        TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(width));
        TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(height));
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1); // Grayscale image
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, static_cast<int>(sizeof(TPixel) * 8));
        TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, TiffSampleFormat<TPixel>());
        TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, width * sizeof(TPixel)));

        // Save resolutions (TIFF resolution is pixels per unit, which is the inverse of spacing)
        TIFFSetField(tiff, TIFFTAG_XRESOLUTION, 1.0 / spacing[0]);
        TIFFSetField(tiff, TIFFTAG_YRESOLUTION, 1.0 / spacing[1]);
        TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        // Save metadata in ImageJ format
        if (slice == 0) { // Add ImageJ metadata to the first slice only
            char description[512];
            snprintf(description, sizeof(description), 
                     "ImageJ=1.53\nimages=%zu\nslices=%zu\nspacing=%.6f\nunit=pixel\nhyperstack=false\nmode=grayscale\nloop=false", 
                     depth, depth, spacing[2]);
            TIFFSetField(tiff, TIFFTAG_IMAGEDESCRIPTION, description);
        }

        for (size_t row = 0; row < height; ++row) {
            if (TIFFWriteScanline(tiff, buffer.data() + row * width, row, 0) < 0) {
                TIFFClose(tiff);
                throw std::runtime_error("Failed to write TIFF scanline.");
            }
        }

        if (slice < depth - 1) {
            if (TIFFWriteDirectory(tiff) == 0) {
                TIFFClose(tiff);
                throw std::runtime_error("Failed to write TIFF directory for slice.");
            }
        }
    }

    TIFFClose(tiff);
}

template <class TImageIn, class TImageOut>
void WriteImageFile(typename TImageIn::Pointer image_in, std::string out_path, bool verbose=false, bool fix_spacings=true, bool scale=true)
{
  typename TImageOut::Pointer image_output = ConvertImage<TImageIn,TImageOut>(image_in, scale);

  if constexpr (TImageOut::ImageDimension == 2)
  {
    SaveImageAsTiff<typename TImageOut::PixelType, TImageOut::ImageDimension>(image_output, out_path);
  }
  else if constexpr (TImageOut::ImageDimension == 3)
  {
    Save3DImageAsTiffStackWithResolutions<typename TImageOut::PixelType, TImageOut::ImageDimension>(image_output, out_path);
  }
  else
  {
    throw std::runtime_error("Unsupported image dimension.");
  }

  if (verbose)
  {
    std::cout << "Wrote " << out_path.c_str() << std::endl;
  }
}
