#include "crop.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include <algorithm>
#include <sstream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  std::string crop_str = "";
  float xy_res = UNSET_FLOAT;
  float step = UNSET_FLOAT;
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: deskew [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("crop,c", po::value<std::string>()->default_value("0,0,0,0,0,0"),"boundary pixel numbers (top,bottom,left,right,front,back)")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("xy-rez,x", po::value<float>(&xy_res)->default_value(-1.0f), "x/y resolution (um/px)")
      ("step,s", po::value<float>(&step)->default_value(-1.0f), "step/interval (um)")
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
      std::cerr << "crop: cropping boundary pixels\n";
      std::cerr << visible_opts << std::endl;
      return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << CROP_VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);

  } catch (po::error& e) {
    std::cerr << "crop: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "crop: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const char* in_path = varsmap["input"].as<std::string>().c_str();
  if (!IsFile(in_path)) {
    std::cerr << "crop: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "crop: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "crop: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  //cropping
  std::string crop_params_str = varsmap["crop"].as<std::string>().c_str();
  std::stringstream ss(crop_params_str);
  std::vector<int> crop_params;
  while( ss.good() )
  {
    std::string substr;
    getline( ss, substr, ',' );
    crop_params.push_back( stoi(substr) );
  }
  if (crop_params.size() < 6)
  {
      for (int i = crop_params.size(); i < 6; i++)
        crop_params.push_back(0);
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "Cropping: Top = " << crop_params[0] << "\n";
    std::cout << "Cropping: Bottom = " << crop_params[1] << "\n";
    std::cout << "Cropping: Left = " << crop_params[2] << "\n";
    std::cout << "Cropping: Right = " << crop_params[3] << "\n";
    std::cout << "Cropping: Front = " << crop_params[4] << "\n";
    std::cout << "Cropping: Back = " << crop_params[5] << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
    std::cout << "X/Y Resolution (um/px) = " << xy_res << "\n";
    std::cout << "Step Size (um) = " << step << "\n";
  }

  // crop
  kImageType::Pointer img = ReadImageFile<kImageType>(in_path);
  kImageType::SpacingType img_spacing = img->GetSpacing();
  if (xy_res > 0.0) {
    img_spacing[0] = xy_res;
    img_spacing[1] = xy_res;
  }
  if (step > 0.0)
    img_spacing[2] = step;
  kImageType::Pointer cropped_img = Crop(img, img_spacing[2], img_spacing[0], crop_params[0], crop_params[1], crop_params[2], crop_params[3], crop_params[4], crop_params[5], verbose);

  img_spacing[0] = 1.0;
  img_spacing[1] = 1.0;
  img_spacing[2] = 1.0;
  cropped_img->SetSpacing(img_spacing);

  // write file
  if (bit_depth == 8) {
    using PixelTypeOut = unsigned char;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(cropped_img, out_path, verbose, false);
  } else if (bit_depth == 16) {
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(cropped_img, out_path, verbose, false);
  } else if (bit_depth == 32) {
    using PixelTypeOut = float;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(cropped_img, out_path, verbose, false);
  } else {
    std::cerr << "crop: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
