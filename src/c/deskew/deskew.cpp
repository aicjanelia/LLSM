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

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <exception>
#include <omp.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <tiffio.hxx>
#include <CImg.h>

namespace cil = cimg_library;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool IsFile(const char* path);

unsigned short GetTIFFBitDepth(const char* path);

template <typename T>
cil::CImg<T> Deskew(cil::CImg<T> img, float angle, float step, float xy_res, T fill_value, bool verbose);

constexpr float lerp(float a, float b, float t) {
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

int main(int argc, char** argv) {
  // parameters
  float xy_res = UNSET_FLOAT;
  float step = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  float fill_value = UNSET_FLOAT;
  int nthreads = UNSET_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: deskew [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("xy-rez,x", po::value<float>(&xy_res)->default_value(0.104f), "x/y resolution (um/px)")
      ("step,s", po::value<float>(&step)->required(), "step/interval (um)")
      ("angle,a", po::value<float>(&angle)->default_value(31.8f), "objective angle from stage normal (degrees)")
      ("fill,f", po::value<float>(&fill_value)->default_value(0.0f), "value used to fill empty deskew regions")
      ("nthreads,n", po::value<int>(&nthreads)->default_value(omp_get_num_procs()),"number of threads")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("overwrite,w", po::value<bool>(&overwrite)->default_value(false)->implicit_value(true)->zero_tokens(), "overwrite output if it exists")
      ("verbose,v", po::value<bool>(&verbose)->default_value(false)->implicit_value(true)->zero_tokens(), "display progress and debug information")
      ("version", "display the version number")
  ;

  po::options_description hidden_opts;
  hidden_opts.add_options()
    ("input", po::value<std::string>()->required(), "input file path")
  ;

  po::positional_options_description positional_opts; 
  positional_opts.add("input", 1);

  po::options_description all_opts;
  all_opts.add(visible_opts).add(hidden_opts);

  // parse options
  po::variables_map varsmap;
  try {
    po::store(po::command_line_parser(argc, argv).options(all_opts).positional(positional_opts).run(), varsmap);

    // print help message
    if (varsmap.count("help") || (argc == 1)) {
      std::cerr << "deskew: removes skewing induced by sample scanning\n";
      std::cerr << visible_opts << std::endl;
      return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);

  } catch (po::error& e) {
    std::cerr << "deskew: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "deskew: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const char* in_path = varsmap["input"].as<std::string>().c_str();
  if (!IsFile(in_path)) {
    std::cerr << "deskew: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }

  // check angle
  if (fabs(angle) > 360.0) {
    std::cerr << "deskew: angle must be within [-360,360]" << std::endl;
    return EXIT_FAILURE;
  }

  // set global openmp thread number
  omp_set_num_threads(nthreads);

  // get output file path
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "deskew: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }

  }

  // get bit depth
  unsigned short bits = GetTIFFBitDepth(in_path);

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "X/Y Resolution (um/px) = " << xy_res << "\n";
    std::cout << "Step Size (um) = " << step << "\n";
    std::cout << "Objective Angle (degrees) = " << angle <<"\n";
    std::cout << "Fill Value = " << fill_value << "\n";
    std::cout << "Number of Threads = " << nthreads << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "\nTIFF Properties\n";
    std::cout << "Bit Depth = " << bits << std::endl;
  }

  // deskew
  float voxel_size[3] = {UNSET_FLOAT};
  cil::CImg<char> tiff_desc;
  if (bits <= 8) {
    cil::CImg<unsigned char> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<unsigned char> deskewed_img = Deskew(img, angle, step, xy_res, (unsigned char) fill_value, verbose);
    deskewed_img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
  } else if (bits == 16) {
    cil::CImg<unsigned short> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<unsigned short> deskewed_img = Deskew(img, angle, step, xy_res, (unsigned short) fill_value, verbose);
    deskewed_img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
  } else if (bits == 32) {
    cil::CImg<float> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<float> deskewed_img = Deskew(img, angle, step, xy_res, fill_value, verbose);
    deskewed_img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
  } else {
    std::cerr << "deskew: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool IsFile(const char* path) {
  fs::path p(path);
  if (fs::exists(p)) {
    if (fs::is_regular_file(p)) {
      return true;
    }
  }
  return false;
}

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
            deskewed_img(xidx, yidx, zidx) = lerp((double) img(Xidx, yidx, zidx),(double) img(Xidx+1, yidx, zidx), weight);

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
