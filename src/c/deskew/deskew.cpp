#include "deskew.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <exception>
#include <omp.h>


constexpr float LinearInterp(float a, float b, float t) {
  // Exact, monotonic, bounded, determinate, and (for a=b=0) consistent:
  if(a<=0 && b>=0 || a>=0 && b<=0) 
    return t*b + (1-t)*a;
  if(t==1)
    return b;

  // exact
  // Exact at t=0, monotonic except near t=1,
  // bounded, determinate, and consistent:
  const float x = a + t*(b-a);
  return t>1 == b>a ? fmax(b,x) : fmin(b,x);  // monotonic near t=1
}

template <typename T>
cil::CImg<T> Deskew(cil::CImg<T> img, float angle, float step, float xy_res, T fill_value, bool verbose) {
  int width = img.width();
  int height = img.height();
  int nslices = img.depth();

  // calculate shift from angle, step, and xy-resolution
  const float shift = step * cos(angle * M_PI/180.0) / xy_res;
  const int deskewed_width = ceil(width + (fabs(shift) * (nslices-1)));
  
  // print parameters
  if (verbose) {
    std::cout << "\nImage Properties\n";
    std::cout << "Width (px) = " << width << "\n";
    std::cout << "Height (px) = " << height << "\n";
    std::cout << "Depth (slices) = " << nslices << "\n";
    std::cout << "\nDeskew Parameters\n";
    std::cout << "Shift (px) = " << shift << "\n";
    std::cout << "Deskewed Image Width (px) = " << deskewed_width << std::endl; 
  }

  // deskew by shifting each slice using 1-D linear interpolation
  cil::CImg<T> deskewed_img(deskewed_width, height, nslices, 1, fill_value);
  const double deskew_origin_x = (deskewed_width-1)/2.0;
  const double origin_x = (width-1)/2.0;
  const double origin_z = (nslices-1)/2.0;

  double start = UNSET_DOUBLE;
  double end = UNSET_DOUBLE;
  if (verbose) {
    start = omp_get_wtime();
  }

  #pragma omp parallel for
    for (int zidx=0; zidx < nslices; ++zidx) {
      for (int yidx=0; yidx < height; ++yidx) {
        for (int xidx=0; xidx < deskewed_width; ++xidx) {
          // map position in deskewed image (xidx) to corresponding position in original image (X)
          // using the image center as the origin
          const double X = (xidx - deskew_origin_x) - shift*(zidx - origin_z) + origin_x;
          const int Xidx = floor(X);
          const double weight = X - Xidx;

          // perform interpolation if mapped position (X) is within the original image bounds
          if (X >= 0.0 && Xidx < (width - 1)) {
            deskewed_img(xidx, yidx, zidx) = LinearInterp((double) img(Xidx, yidx, zidx),(double) img(Xidx+1, yidx, zidx), weight);

          // if the mapping (X) lands within EPSILON of the original image edge, fill use the original image edge value
          } else if (weight < EPSILON && Xidx == (width - 1)) {
            deskewed_img(xidx, yidx, zidx) = img(Xidx, yidx, zidx);          
          }
        }
      }
    }
  
  if (verbose) {
    end = omp_get_wtime();
    std::cout << "\nElapsed Deskew Time (sec) = " << end - start << std::endl;
  }

  return deskewed_img;
}
