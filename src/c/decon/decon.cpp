#include "decon.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "resampler.h"
#include "math_local.h"
#include "writer.h"
#include <algorithm>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  float xy_res = UNSET_FLOAT;
  float kernel_zstep = UNSET_FLOAT;
  float img_zstep = UNSET_FLOAT;
  float subtract_constant = UNSET_FLOAT;
  unsigned int iterations = UNSET_UNSIGNED_INT;
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: decon [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("kernel,k", po::value<std::string>()->required(),"kernel file path")
      ("iterations,n", po::value<unsigned int>(&iterations)->required(),"deconvolution iterations")
      ("xy-rez,x", po::value<float>(&xy_res)->default_value(-1.0f), "x/y resolution (um/px)")
      ("kernel-spacing,p", po::value<float>(&kernel_zstep)->default_value(-1.0f),"z-step size of kernel")
      ("image-spacing,q", po::value<float>(&img_zstep)->default_value(-1.0f),"z-step size of input image")
      ("subtract-constant,s", po::value<float>(&subtract_constant)->default_value(0.0f),"constant intensity value to subtract from input image")
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
      std::cerr << "decon: deconvolves an image with a PSF or PSF parameters\n";
      std::cerr << visible_opts << std::endl;
      return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << DECON_VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);

  } catch (po::error& e) {
    std::cerr << "decon: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "decon: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const char* in_path = varsmap["input"].as<std::string>().c_str();
  if (!IsFile(in_path)) {
    std::cerr << "decon: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* kernel_path = varsmap["kernel"].as<std::string>().c_str();
  if (!IsFile(kernel_path)) {
    std::cerr << "decon: kernel path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "decon: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "decon: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "Iterations = " << iterations << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Kernel Path = " << kernel_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
  }

  // read data
  kImageType::Pointer img = ReadImageFile<kImageType>(in_path);
  kImageType::Pointer kernel = ReadImageFile<kImageType>(kernel_path);

  // set spacing
  kImageType::SpacingType img_spacing = img->GetSpacing();
  kImageType::SpacingType kernel_spacing = kernel->GetSpacing();

  if (xy_res > 0.0) {
    img_spacing[0] = xy_res;
    img_spacing[1] = xy_res;
    kernel_spacing[0] = xy_res;
    kernel_spacing[1] = xy_res;
  }
  if (img_zstep > 0.0)
    img_spacing[2] = img_zstep;
  if (kernel_zstep > 0.0)
    kernel_spacing[2] = kernel_zstep;
  
  img->SetSpacing(img_spacing);
  kernel->SetSpacing(kernel_spacing);

  // resample kernel
  if (img_spacing[2] != kernel_spacing[2])
  {
    kernel = Resampler(kernel, img_spacing, verbose);
  }

  // subtract constant
  if (subtract_constant != 0.0)
  {
    img = SubtractConstantClamped(img, (kPixelType) subtract_constant/std::numeric_limits<unsigned short>::max()); // TODO: scale subtraction by input type

  }

  // decon
  kImageType::Pointer decon_img = RichardsonLucy(img, kernel, iterations, verbose);

  decon_img->SetSpacing(img_spacing);

  // write file
  if (bit_depth == 8) {
    using PixelTypeOut = unsigned char;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(decon_img, out_path);
  } else if (bit_depth == 16) {
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(decon_img, out_path);
  } else if (bit_depth == 32) {
    using PixelTypeOut = float;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(decon_img, out_path);
  } else {
    std::cerr << "decon: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}