#define VERSION "AIC Deskew version 0.1.0"
#define UNSET_FLOAT -1.0f
#define UNSET_INT -1
#define UNSET_UNSIGNED_SHORT 0
#define UNSET_BOOL false
#define cimg_use_tiff

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <tiffio.hxx>
#include <CImg.h>

namespace cil = cimg_library;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool IsFileTIFF(const char* path);

unsigned short GetTIFFBitDepth(const char* path);

template <typename T>
void LoadTIFF(const char* path, cil::CImg<T>& img);

// int SaveImage(cil::CImg img, std::string path);

// template <typename T>
// cil::CImg Deskew(cil::CImg<T> img, float xy_rez, float z_rez, float angle, int nthreads);

int main(int argc, char** argv) {
  // parameters
  float xy = UNSET_FLOAT;
  float z = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  int nthreads = UNSET_INT;

  // declare the supported options
  po::options_description visible_opts("usage: deskew [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("xy-rez,x", po::value<float>(&xy)->default_value(0.104f), "x/y resolution (um/px)")
      ("z-rez,z", po::value<float>(&z)->default_value(0.25f), "z resolution (um/px)")
      ("angle,a", po::value<float>(&angle)->default_value(32.8f), "deskewing angle (degrees)")
      ("nthreads,n", po::value<int>(&nthreads)->default_value(4),"number of threads")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("version,v", "display the version number")
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
  if (!IsFileTIFF(in_path)) {
    std::cerr << "deskew: not a TIFF file" << std::endl;
    return EXIT_FAILURE;
  }

  const char* out_path = varsmap["output"].as<std::string>().c_str();

  // get bit depth
  unsigned short bits = GetTIFFBitDepth(in_path);

  // deskew
  if (bits <= 8) {
    cil::CImg<unsigned char> img;
    img.load_tiff(in_path);
    img.save_tiff(out_path, 0, 0, "description", false);
  } else if (bits == 16) {
    cil::CImg<unsigned short> img;
    img.load_tiff(in_path);
    img.save_tiff(out_path, 0, 0, "description", false);
  } else if (bits == 32) {
    cil::CImg<float> img;
    img.load_tiff(in_path);
    img.save_tiff(out_path, 0, 0, "description", false);
  } else {
    std::cerr << "deskew: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool IsFileTIFF(const char* path) {
  fs::path p(path);
  if (fs::exists(p)) {
    if (fs::is_regular_file(p)){
      //TODO: Check if file is a tiff and not just a file
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

// template <typename T>
// void LoadTIFF(const char* path, cil::CImg<T>& img) {
//   // load TIFF image
//   try {
//     img.load_tiff(path);
//   } catch (cil::CImgException& e) {
//     std::cerr << "CImg Library: " << e.what() << std::endl;
//     throw e;
//   }
// }

// template <typename T>
// cil::CImg Deskew(cil::CImg<T> img, float xy_rez, float z_rez, float angle, int nthreads) {
//   // int deskew(const CImg<> &inBuf, double deskewFactor, CImg<> &outBuf, int extraShift)
// // {
// //   auto nz = inBuf.depth();
// //   auto ny = inBuf.height();
// //   auto nx = inBuf.width();
// //   auto nxOut = outBuf.width();
// // #pragma omp parallel for
// //   for (auto zout=0; zout < nz; zout++)
// //     for (auto yout=0; yout < ny; yout++)
// //       for (auto xout=0; xout < nxOut; xout++) {
// //         const double xin = (xout - nxOut/2. + extraShift) - deskewFactor*(zout-nz/2.) + nx/2.;

// //         if (xin >= 0 && floor(xin) <= nx-1) {
// //           const double offset = xin - floor(xin);
// //           const int floor_xin = floor(xin);
// //           outBuf(xout, yout, zout) = (1.0 - offset) * inBuf(floor_xin, yout, zout)
// //             + offset * inBuf(floor_xin+1, yout, zout);
// //         }
// //         else
// //           outBuf(xout, yout, zout) = 100.f;
// //   }
// // }

//   return img;
// }
