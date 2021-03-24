#pragma once

#define DESKEW_VERSION "AIC Deskew version 0.1.0"

#include "defines.h"
#include <itkImage.h>

template <typename T>
itk::SmartPointer<kImageType> Deskew(itk::SmartPointer<kImageType> img, float angle, float step, float xy_res, T fill_value, bool verbose) {

  // // calculate shift from angle, step, and xy-resolution
  // const float shift = step * cos(angle * M_PI/180.0) / xy_res;
  // const int deskewed_width = ceil(width + (fabs(shift) * (nslices-1)));
  
  // // print parameters
  // if (verbose) {
  //   std::cout << "\nImage Properties\n";
  //   std::cout << "Width (px) = " << width << "\n";
  //   std::cout << "Height (px) = " << height << "\n";
  //   std::cout << "Depth (slices) = " << nslices << "\n";
  //   std::cout << "\nDeskew Parameters\n";
  //   std::cout << "Shift (px) = " << shift << "\n";
  //   std::cout << "Deskewed Image Width (px) = " << deskewed_width << std::endl; 
  // }

  // deskew by shifting each slice using 1-D linear interpolation
  // cil::CImg<T> deskewed_img(deskewed_width, height, nslices, 1, fill_value);
  // const double deskew_origin_x = (deskewed_width-1)/2.0;
  // const double origin_x = (width-1)/2.0;
  // const double origin_z = (nslices-1)/2.0;

  return nullptr;
}