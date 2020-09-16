#define VERSION "AIC Deskew version 0.1.0"
#define cimg_display  1
#define UNSET_FLOAT -1.0f
#define UNSET_INT -1
#define UNSET_BOOL false

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <CImg.h>

using namespace cimg_library;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char *argv[]) {
  // parameters
  float xy = UNSET_FLOAT;
  float z = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  int nthreads = UNSET_INT;

  // declare the supported options
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "display this help message")
      ("xy-rez,x", po::value<float>(&xy)->default_value(0.104f), "x/y resolution (um/px)")
      ("z-rez,z", po::value<float>(&z)->default_value(0.25f), "z resolution (um/px)")
      ("angle,a", po::value<float>(&angle)->default_value(32.8f), "deskewing angle (degrees)")
      ("nthreads,n", po::value<int>(&nthreads)->default_value(4),"number of threads")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("input,i", po::value<std::string>()->required(), "input file path")
      ("version,v", "display the version number")
  ;

  po::positional_options_description posdesc; 
  posdesc.add("input,i", 1);

  // parse options
  po::variables_map varsmap;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posdesc).run(), varsmap);

    // print help message
    if (varsmap.count("help") || (argc == 1)) {
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);
  } catch (po::error &e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Error: unknown error during command line parsing\n\n";
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  }

  // check input file
  fs::path inPath(varsmap["input"].as<std::string>());  // avoid repeated path construction below

  if (fs::exists(inPath)) {
    if (fs::is_regular_file(inPath)){
      std::cout << "Processing " << inPath << std::endl;
    }
  } else {
    std::cerr << "Error: " << inPath << " does not exist" << std::endl;
  }


//   CImg<unsigned char> img(640,400,1,3);  // Define a 640x400 color image with 8 bits per color component.
//   img.fill(0);                           // Set pixel values to 0 (color : black)
//   unsigned char purple[] = { 255,0,255 };        // Define a purple color
//   img.draw_text(100,100,"Hello World",purple); // Draw a purple "Hello world" at coordinates (100,100).
//   img.display("My first CImg code");             // Display the image in a display window.
//   return 0;

  return EXIT_SUCCESS;
}

// int deskew(const CImg<> &inBuf, double deskewFactor, CImg<> &outBuf, int extraShift)
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