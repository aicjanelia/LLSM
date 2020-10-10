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
cil::CImg<T> Deskew(cil::CImg<T> img, float angle, float step, float xy_res, int nthreads, bool verbose);

int main(int argc, char** argv) {
  // parameters
  float xy_res = UNSET_FLOAT;
  float step = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  int nthreads = UNSET_INT;
  bool verbose = false;

  // declare the supported options
  po::options_description visible_opts("usage: deskew [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("xy-rez,x", po::value<float>(&xy_res)->default_value(0.104f), "x/y resolution (um/px)")
      ("step,s", po::value<float>(&step)->required(), "step/interval (um)")
      ("angle,a", po::value<float>(&angle)->default_value(31.8f), "objective angle from stage normal (degrees)")
      ("nthreads,n", po::value<int>(&nthreads)->default_value(4),"number of threads")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("verbose,v", po::value<bool>(&verbose)->implicit_value(true), "display progress and debug information")
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
  if (!IsFileTIFF(in_path)) {
    std::cerr << "deskew: not a TIFF file" << std::endl;
    return EXIT_FAILURE;
  }

  // check angle
  if (fabs(angle) > 360.0) {
    std::cerr << "deskew: angle must be within [-360,360]" << std::endl;
    return EXIT_FAILURE;
  }

  // get output file path
  const char* out_path = varsmap["output"].as<std::string>().c_str();

  // get bit depth
  unsigned short bits = GetTIFFBitDepth(in_path);

  // print parameters
  if (verbose) {
    std::cout << "Input Parameters:\n";
    std::cout << "X/Y Resolution (um/px) = " << xy_res << "\n";
    std::cout << "Step Size (um) = " << step << "\n";
    std::cout << "Objective Angle (degrees) = " << angle <<"\n";
    std::cout << "Number of Threads = " << nthreads << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n\n";
    std::cout << "Checking Bit Depth...\n";
    std::cout << "Bit Depth = " << bits << std::endl;
  }

  // deskew
  float voxel_size[3] = {UNSET_FLOAT};
  cil::CImg<char> tiff_desc;
  if (bits <= 8) {
    cil::CImg<unsigned char> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<unsigned char> deskewed_img = Deskew(img, angle, step, xy_res, nthreads, verbose);
    img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
  } else if (bits == 16) {
    cil::CImg<unsigned short> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<unsigned short> deskewed_img = Deskew(img, angle, step, xy_res, nthreads, verbose);
    img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
  } else if (bits == 32) {
    cil::CImg<float> img;
    img.load_tiff(in_path, 0, ~0U, 1, voxel_size, &tiff_desc);
    cil::CImg<float> deskewed_img = Deskew(img, angle, step, xy_res, nthreads, verbose);
    img.save_tiff(out_path, 0, voxel_size, tiff_desc, false);
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

template <typename T>
cil::CImg<T> Deskew(cil::CImg<T> img, float angle, float step, float xy_res, int nthreads, bool verbose) {
  int width = img.width();
  int height = img.height();
  int nslices = img.depth();

  // calculate shift from angle, step, and xy-resolution
  float shift = step * cos(angle * M_PI/180.0) / xy_res;
  int deskewed_width = ceil(width + (fabs(shift) * (nslices - 1)));
  
  if (verbose) {
    
  }

  // deskew by shifting each slice using 1-D linear interpolation

//   int deskew(const CImg<> &inBuf, double deskewFactor, CImg<> &outBuf, int extraShift)
// {
//   auto nz = inBuf.depth();
//   auto ny = inBuf.height();
//   auto nx = inBuf.width();
//   auto nxOut = outBuf.width();
// #pragma omp parallel for
//   for (auto zout=0; zout < nz; zout++)
//     for (auto yout=0; yout < ny; yout++)
//       for (auto xout=0; xout < nxOut; xout++) {
//         const double xin = (xout - nxOut/2. + extraShift) - deskewFactor*(zout-nz/2.) + nx/2.;

//         if (xin >= 0 && floor(xin) <= nx-1) {
//           const double offset = xin - floor(xin);
//           const int floor_xin = floor(xin);
//           outBuf(xout, yout, zout) = (1.0 - offset) * inBuf(floor_xin, yout, zout)
//             + offset * inBuf(floor_xin+1, yout, zout);
//         }
//         else
//           outBuf(xout, yout, zout) = 100.f;
//   }
// }

  return img;
}
