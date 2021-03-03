#define VERSION "AIC Deskew version 0.1.0"
#define UNSET_DOUBLE -1.0
#define UNSET_FLOAT -1.0f
#define UNSET_INT -1
#define UNSET_UNSIGNED_SHORT 0
#define UNSET_BOOL false
#define EPSILON 1e-5
#define cimg_display 0
#define cimg_verbosity 1
#define cimg_use_tiff
#define cimg_use_openmp 1

#include <CImg.h>

namespace cil = cimg_library;


bool IsFile(const char* path);

unsigned short GetTIFFBitDepth(const char* path);

template <typename T>
cil::CImg<T> Deskew(cil::CImg<T> img, float angle, float step, float xy_res, T fill_value, bool verbose);