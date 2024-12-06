#include "mip.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include "resampler.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  bool x_axis = UNSET_BOOL;
  bool y_axis = UNSET_BOOL;
  bool z_axis = UNSET_BOOL;
  float xy_res = UNSET_FLOAT;
  float z_res = UNSET_FLOAT;
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: mip [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("x-axis,x", po::value<bool>(&x_axis)->default_value(false)->implicit_value(true)->zero_tokens(), "generate x-axis projection")
      ("y-axis,y", po::value<bool>(&y_axis)->default_value(false)->implicit_value(true)->zero_tokens(), "generate y-axis projection")
      ("z-axis,z", po::value<bool>(&z_axis)->default_value(false)->implicit_value(true)->zero_tokens(), "generate z-axis projection")
      ("xy-rez,p", po::value<float>(&xy_res)->default_value(0.104f), "x/y resolution (um/px)")
      ("z-rez,q", po::value<float>(&z_res)->default_value(0.104f), "z resolution (um/px)")
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
      std::cerr << "mip: generates a maximum intensity projection along the specified axes\n";
      std::cerr << visible_opts << std::endl;
      return EXIT_FAILURE;
    }

    // print version number
    if (varsmap.count("version")) {
      std::cerr << MIP_VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);

  } catch (po::error& e) {
    std::cerr << "mip: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "mip: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const char* in_path = varsmap["input"].as<std::string>().c_str();
  if (!IsFile(in_path)) {
    std::cerr << "mip: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "mip: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "mip: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  // check at least one axis is true
  if (!x_axis & !y_axis & !z_axis)
  {
    std::cerr << "mip: at least one axis projection (x, y, or z) must been enabled" << std::endl;
    return EXIT_FAILURE;
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "X-axis = " << x_axis << "\n";
    std::cout << "Y-axis = " << y_axis << "\n";
    std::cout << "Z-axis = " << z_axis << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
  }

  // mip
  kImageType::Pointer img = ReadImageFile<kImageType>(in_path);

  bool axes[] = {x_axis, y_axis, z_axis};
  std::string labels[] = {"_x", "_y", "_z"};

  // resample the image so the voxels are cubes before making the projections
  kImageType::SpacingType img_spacing;

  img_spacing[0] = xy_res;
  img_spacing[1] = xy_res;
  img_spacing[2] = z_res;
  img->SetSpacing(img_spacing);

  if (z_res != xy_res)
  {
    img = Resampler(img, xy_res, verbose);
  }

  for (unsigned int i = 0; i < 3; ++i) 
  {
    if (axes[i])
    {
        std::string axis_out_path = AppendPath(out_path, labels[i]);
        using ProjectionType = itk::Image<kPixelType, 2>;
        ProjectionType::Pointer mip_img = MaxIntensityProjection(img, i, verbose);
        ProjectionType::SpacingType mip_spacing;

        mip_spacing[0] = 1.0;
        mip_spacing[1] = 1.0;
        mip_img->SetSpacing(mip_spacing);

        // write file
        if (bit_depth == 8) {
            using PixelTypeOut = unsigned char;
            using ImageTypeOut = itk::Image<PixelTypeOut, 2>;
            WriteImageFile<ProjectionType,ImageTypeOut>(mip_img, axis_out_path, verbose, false);
        } else if (bit_depth == 16) {
            using PixelTypeOut = unsigned short;
            using ImageTypeOut = itk::Image<PixelTypeOut, 2>;
            WriteImageFile<ProjectionType,ImageTypeOut>(mip_img, axis_out_path, verbose, false);
        } else if (bit_depth == 32) {
            using PixelTypeOut = float;
            using ImageTypeOut = itk::Image<PixelTypeOut, 2>;
            WriteImageFile<ProjectionType,ImageTypeOut>(mip_img, axis_out_path, verbose, false);
        } else {
            std::cerr << "mip: unknown bit depth" << std::endl;
            return EXIT_FAILURE;
        }
    }
  }

  return EXIT_SUCCESS;
}
