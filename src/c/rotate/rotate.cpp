#include "rotate.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>
#include <boost/program_options.hpp>
#include <itkMultiThreaderBase.h>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  float xy_res = UNSET_FLOAT;
  float z_res = UNSET_FLOAT;
  float angle = UNSET_FLOAT;
  float shift = UNSET_FLOAT;
  float fill_value = UNSET_FLOAT;
  std::string crop_mode = "keep-all";
  std::string z_trim_str = "0";
  std::string output_spacing_str = "";
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
  unsigned int threadnum = UNSET_UNSIGNED_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: rotate [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("angle,a", po::value<float>(&angle)->required(), "rotation angle about the y axis (degrees), BigStitcher y-axis convention (MOSAIC: -32.45)")
      ("xy-res,x", po::value<float>(&xy_res)->default_value(-1.0f), "x/y resolution of the input (um/px); default is read from the input file")
      ("z-res,z", po::value<float>(&z_res)->default_value(-1.0f), "z resolution of the input (um/px), i.e. step*sin(angle) after deskewing; default is read from the input file")
      ("shift,d", po::value<float>(&shift)->default_value(0.0f), "signed deskew shift (px per z plane) = step*cos(deskew angle)/xy-res; needed for strict cropping, 0 treats the whole array as valid")
      ("crop-mode,c", po::value<std::string>(&crop_mode)->default_value("keep-all"), "keep-all (bounding box of the valid data) or strict (largest fully valid region; needs --shift and a matching angle)")
      ("z-trim", po::value<std::string>(&z_trim_str)->default_value("0"), "output z slices to remove after cropping, to avoid edge effects: N (both ends) or bottom,top (e.g. 2,3)")
      ("output-spacing", po::value<std::string>(&output_spacing_str)->default_value(""), "output voxel spacing (um): S (cubic voxels), X,Y,Z, or 'input' to keep the input spacing; default is cubic voxels the size of the input xy pixel")
      ("fill,f", po::value<float>(&fill_value)->default_value(0.0f), "value used to fill empty regions")
      ("output,o", po::value<std::string>()->required(),"output file path")
      ("bit-depth,b", po::value<unsigned int>(&bit_depth)->default_value(16),"bit depth (8, 16, or 32) of output image")
      ("thread,t", po::value<unsigned int>(&threadnum)->default_value(1),"number of threads")
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
      std::cerr << "rotate: rotates an image stack about the y axis (e.g., from the objective to the coverslip frame)\n";
      std::cerr << visible_opts << std::endl;
      return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << ROTATE_VERSION << std::endl;
      return EXIT_FAILURE;
    }

    // check options
    po::notify(varsmap);

    // set thread number
    itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(threadnum);

  } catch (po::error& e) {
    std::cerr << "rotate: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "rotate: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const std::string in_path_str = varsmap["input"].as<std::string>();
  const std::string out_path_str = varsmap["output"].as<std::string>();
  const char* in_path = in_path_str.c_str();
  const char* out_path = out_path_str.c_str();
  if (!IsFile(in_path)) {
    std::cerr << "rotate: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "rotate: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "rotate: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  // check angle
  if (!(fabs(angle) <= 360.0)) {
    std::cerr << "rotate: angle must be within [-360,360]" << std::endl;
    return EXIT_FAILURE;
  }

  // check crop mode
  llsm_rotate::CropMode mode;
  if (crop_mode == "keep-all") {
    mode = llsm_rotate::CropMode::KeepAll;
  } else if (crop_mode == "strict") {
    mode = llsm_rotate::CropMode::Strict;
  } else {
    std::cerr << "rotate: crop mode must be keep-all or strict" << std::endl;
    return EXIT_FAILURE;
  }

  // check z trim: "N" or "bottom,top"
  std::array<long, 2> z_trim = {0, 0};
  {
    const auto comma = z_trim_str.find(',');
    try {
      std::size_t used = 0;
      const std::string first = z_trim_str.substr(0, comma);
      z_trim[0] = std::stol(first, &used);
      bool ok = (used == first.size());
      if (comma == std::string::npos) {
        z_trim[1] = z_trim[0];
      } else {
        const std::string second = z_trim_str.substr(comma + 1);
        z_trim[1] = std::stol(second, &used);
        ok = ok && (used == second.size());
      }
      if (!ok || z_trim[0] < 0 || z_trim[1] < 0)
        throw std::invalid_argument("z-trim");
    } catch (const std::exception&) {
      std::cerr << "rotate: z-trim must be a non-negative integer N or two integers bottom,top" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // without --shift the whole array is treated as valid data, so the empty deskew triangles are
  // rotated and counted in the keep-all bounding box (many extra, empty z slices on deskewed input)
  if (varsmap["shift"].defaulted()) {
    std::cerr << "rotate: warning: --shift not given; treating the whole array as valid data. "
                 "If the input was deskewed, the empty deskew triangles will be included in the "
                 "output (many empty z slices). Pass --shift step*cos(deskew angle)/xy-res "
                 "(or --shift 0 to silence this warning)." << std::endl;
  }

  // check output spacing: "", "input", "S" or "X,Y,Z" (resolved against the input spacing below)
  const bool keep_input_spacing = (output_spacing_str == "input");
  std::vector<double> user_spacing;
  if (!output_spacing_str.empty() && !keep_input_spacing) {
    try {
      std::size_t start = 0;
      while (true) {
        const auto comma = output_spacing_str.find(',', start);
        const std::string token = output_spacing_str.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        std::size_t used = 0;
        const double value = std::stod(token, &used);
        // plain decimal numbers only (stod would also accept "nan"/"inf")
        if (token.empty() || !(std::isdigit(static_cast<unsigned char>(token[0])) || token[0] == '.') ||
            used != token.size() || !(value > 0.0 && value < 1.0e6))
          throw std::invalid_argument("output-spacing");
        user_spacing.push_back(value);
        if (comma == std::string::npos)
          break;
        start = comma + 1;
      }
      if (user_spacing.size() != 1 && user_spacing.size() != 3)
        throw std::invalid_argument("output-spacing");
    } catch (const std::exception&) {
      std::cerr << "rotate: output-spacing must be 'input', a positive number S, or three positive numbers X,Y,Z (um)" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "X/Y Resolution (um/px) = " << xy_res << "\n";
    std::cout << "Z Resolution (um/px) = " << z_res << "\n";
    std::cout << "Rotation Angle (degrees) = " << angle << "\n";
    std::cout << "Deskew Shift (px/plane) = " << shift << "\n";
    std::cout << "Crop Mode = " << crop_mode << "\n";
    std::cout << "Z Trim (bottom,top slices) = " << z_trim[0] << "," << z_trim[1] << "\n";
    std::cout << "Output Spacing = " << (output_spacing_str.empty() ? "xy (cubic)" : output_spacing_str) << "\n";
    std::cout << "Fill Value = " << fill_value << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
  }

  // rotate
  kImageType::Pointer img = ReadImageFile<kImageType>(in_path);

  kImageType::SpacingType img_spacing = img->GetSpacing();
  if (xy_res > 0.0)
    img_spacing[0] = xy_res;
  if (z_res > 0.0)
    img_spacing[2] = z_res;
  // xy spacing is one value for both axes; take x unless y was set differently in the file and x was given
  img_spacing[1] = img_spacing[0];

  std::array<double, 3> out_spacing;
  if (keep_input_spacing) {
    out_spacing = {img_spacing[0], img_spacing[1], img_spacing[2]};
  } else if (user_spacing.size() == 1) {
    out_spacing = {user_spacing[0], user_spacing[0], user_spacing[0]};
  } else if (user_spacing.size() == 3) {
    out_spacing = {user_spacing[0], user_spacing[1], user_spacing[2]};
  } else {
    out_spacing = {img_spacing[0], img_spacing[0], img_spacing[0]}; // default: cubic at the input xy size
  }

  kImageType::Pointer rotated_img;
  try {
    rotated_img = Rotate(img, angle, img_spacing[0], img_spacing[2], shift, mode, out_spacing,
                         (kPixelType) fill_value/std::numeric_limits<unsigned short>::max(), verbose, z_trim); // same fill scaling as deskew
  } catch (const std::invalid_argument& e) {
    std::cerr << "rotate: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // write file
  if (bit_depth == 8) {
    using PixelTypeOut = unsigned char;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(rotated_img, out_path, verbose, false);
  } else if (bit_depth == 16) {
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(rotated_img, out_path, verbose, false);
  } else if (bit_depth == 32) {
    using PixelTypeOut = float;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(rotated_img, out_path, verbose, false);
  } else {
    std::cerr << "rotate: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
