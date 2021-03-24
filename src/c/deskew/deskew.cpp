#include "deskew.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include <algorithm>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  float xy_res = UNSET_FLOAT;
  float step = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  float fill_value = UNSET_FLOAT;
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
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
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("bit-depth,b", po::value<unsigned int>(&bit_depth)->default_value(16),"bit depth (8, 16, or 32) of output image")
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
      std::cerr << DESKEW_VERSION << std::endl;
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
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "deskew: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "deskew: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  // check angle
  if (fabs(angle) > 360.0) {
    std::cerr << "deskew: angle must be within [-360,360]" << std::endl;
    return EXIT_FAILURE;
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "X/Y Resolution (um/px) = " << xy_res << "\n";
    std::cout << "Step Size (um) = " << step << "\n";
    std::cout << "Objective Angle (degrees) = " << angle <<"\n";
    std::cout << "Fill Value = " << fill_value << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
  }

  // deskew
  itk::SmartPointer<kImageType> img = ReadImageFile(in_path);
  itk::SmartPointer<kImageType> deskew_img = Deskew(img, angle, step, xy_res, (unsigned char) fill_value, verbose);

  // write file
  if (bit_depth == 8) {
    using PixelTypeOut = unsigned char;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(deskew_img, out_path);
  } else if (bit_depth == 16) {
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(deskew_img, out_path);
  } else if (bit_depth == 32) {
    using PixelTypeOut = float;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(deskew_img, out_path);
  } else {
    std::cerr << "deskew: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
