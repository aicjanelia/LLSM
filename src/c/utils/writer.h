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

    TIFF* tiff = TIFFOpen(filename.c_str(), "w");
    if (!tiff) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    // Set TIFF fields
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(width));
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(height));
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1); // Grayscale image
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, sizeof(TPixel) * 8);
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, width * sizeof(TPixel)));

    TIFFSetField(tiff, TIFFTAG_XRESOLUTION, spacing[0]);
    TIFFSetField(tiff, TIFFTAG_YRESOLUTION, spacing[1]);
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

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

    TIFF* tiff = TIFFOpen(filename.c_str(), "w");
    if (!tiff) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    for (size_t slice = 0; slice < depth; ++slice) {
        std::vector<TPixel> buffer(width * height);
        typename ImageType::IndexType start = { {startIndex[0], startIndex[1], startIndex[2] + slice} };
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
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, sizeof(TPixel) * 8);
        TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, width * sizeof(TPixel)));

        // Save resolutions
        TIFFSetField(tiff, TIFFTAG_XRESOLUTION, spacing[0]);
        TIFFSetField(tiff, TIFFTAG_YRESOLUTION, spacing[1]);
        TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        // Save Z resolution in ImageJ format
        if (slice == 0) { // Add spacing information to the first slice only
            char description[128];
            snprintf(description, sizeof(description), "spacing=%.6f", spacing[2]);
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
