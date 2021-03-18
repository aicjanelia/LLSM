#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>
#include <tiffio.hxx>

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