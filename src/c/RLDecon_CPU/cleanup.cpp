#define cimg_use_openmp
#define cimg_use_tiff
#define cimg_use_cpp11 1

#include "CImg.h"
using namespace cimg_library;


int main(int argc, char* argv[])
{

  CImg<unsigned short> raw_image(argv[1]);
  cimg_forXYZ(raw_image, x, y, z)
    raw_image(x,y,z) = raw_image(x,y,z) < 60000 ? raw_image(x,y,z) : 0.;

  raw_image.save_tiff(argv[1]);
}
