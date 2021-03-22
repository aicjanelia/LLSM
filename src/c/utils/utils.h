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
#include <itkRescaleIntensityImageFilter.h>


template <class TImageIn, class TImageOut>
itk::SmartPointer<TImageOut> ConvertImage(itk::SmartPointer<TImageIn> image_in, const double output_min=0.0, const double output_max=1.0)
{
  using RescaleType = itk::RescaleIntensityImageFilter<TImageIn, TImageOut>;
  typename RescaleType::Pointer rescale = RescaleType::New();
  rescale->SetInput(image_in);
  rescale->SetOutputMinimum(output_min);
  rescale->SetOutputMaximum(output_max);
  rescale->Update();

  return rescale->GetOutput();
}

namespace fs = boost::filesystem;

bool IsFile(const char* path) {
  fs::path p(path);
  if (fs::exists(p)) {
    if (fs::is_regular_file(p)) {
      return true;
    }
  }
  return false;
}

// TODO: remove this function and tiffio header once deskew uses ITK
unsigned short GetTIFFBitDepth(const char* path) {
  // read bit depth from file
  unsigned short bps = UNSET_UNSIGNED_SHORT;
  TIFF* tif = TIFFOpen(path, "r");
  int defined = TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFClose(tif);
  if (!defined) {
    std::cerr << "GetTIFFBitDepth: no BITPERSAMPLE tag found" << std::endl;
  }
  return bps;
}