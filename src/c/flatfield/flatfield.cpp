#include "flatfield.h"
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
  float img_zstep = UNSET_FLOAT;
  unsigned int bit_depth = UNSET_UNSIGNED_INT;
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: flatfield [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("dark,d", po::value<std::string>()->required(),"dark image file path")
      ("n-image,n", po::value<std::string>()->required(),"N image file path")
      ("xy-rez,x", po::value<float>(&xy_res)->default_value(-1.0f), "x/y resolution (um/px)")
      ("image-spacing,q", po::value<float>(&img_zstep)->default_value(-1.0f),"z-step size of input image")
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
      std::cerr << FLATFIELD_VERSION << std::endl;
      return EXIT_FAILURE;
    }
    
    // check options
    po::notify(varsmap);

  } catch (po::error& e) {
    std::cerr << "flatfield: " << e.what() << "\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "flatfield: unknown error during command line parsing\n\n";
    std::cerr << visible_opts << std::endl;
    return EXIT_FAILURE;
  }

  // check files
  const char* in_path = varsmap["input"].as<std::string>().c_str();
  if (!IsFile(in_path)) {
    std::cerr << "flatfield: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* dark_path = varsmap["dark"].as<std::string>().c_str();
  if (!IsFile(dark_path)) {
    std::cerr << "flatfield: dark image path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* n_path = varsmap["n-image"].as<std::string>().c_str();
  if (!IsFile(n_path)) {
    std::cerr << "flatfield: N image path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "flatfield: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }
  }

  // check bit depth
  unsigned int bits[] = {8, 16, 32};
  unsigned int* p = std::find(std::begin(bits), std::end(bits), bit_depth);
  if (p == std::end(bits)) {
    std::cerr << "flatfield: bit depth must be 8, 16, or 32" << std::endl;
    return EXIT_FAILURE;
  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Dark Image Path = " << dark_path << "\n";
    std::cout << "N Image Path = " << n_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << std::endl;
  }

  // read data
  kImageType::Pointer img = ReadImageFile<kImageType>(in_path, false, false);

  //itk::Image<kPixelType, 2>::Pointer dark_tmp = ReadImageFile<itk::Image<kPixelType, 2>>(dark_path);

  itk::ImageIOBase::Pointer image_io = itk::ImageIOFactory::CreateImageIO(dark_path, itk::CommonEnums::IOFileMode::ReadMode);
  image_io->SetFileName(dark_path);
  image_io->ReadImageInformation();
  const itk::IOComponentEnum component_type = image_io->GetComponentType();
  kSliceType::Pointer dark = ReadImage<2, itk::Image<kPixelType, 2>>(dark_path, component_type, false);
  //kImageType::Pointer dark = Convert2DImageTo3D<kPixelType>(dark_tmp);

  //itk::Image<kPixelType, 2>::Pointer n_img_tmp = ReadImageFile<itk::Image<kPixelType, 2>>(n_path);
  //kImageType::Pointer n_img = Convert2DImageTo3D<kPixelType>(n_img_tmp);

  //kImageType::Pointer dark = ReadImageFile<kImageType>(dark_path);
  //kImageType::Pointer n_img = ReadImageFile<kImageType>(n_path);

  itk::ImageIOBase::Pointer image_io2 = itk::ImageIOFactory::CreateImageIO(n_path, itk::CommonEnums::IOFileMode::ReadMode);
  image_io2->SetFileName(n_path);
  image_io2->ReadImageInformation();
  const itk::IOComponentType component_type2 = image_io2->GetComponentType();
  kSliceType::Pointer n_img = ReadImage<2, itk::Image<kPixelType, 2>>(n_path, component_type2, false);
  //kImageType::Pointer n_img = Convert2DImageTo3D<kPixelType>(n_img_tmp);

  // set spacing
  kImageType::SpacingType img_spacing = img->GetSpacing();
  if (xy_res > 0.0) {
    img_spacing[0] = xy_res;
    img_spacing[1] = xy_res;
  }
  if (img_zstep > 0.0)
    img_spacing[2] = img_zstep;
  
  img->SetSpacing(img_spacing);

  kSliceType::SpacingType slice_spacing = dark->GetSpacing();
  slice_spacing[0] = img_spacing[0];
  slice_spacing[1] = img_spacing[1];
  dark->SetSpacing(slice_spacing);
  n_img->SetSpacing(slice_spacing);


  using MinMaxFilterType = itk::MinimumMaximumImageFilter<kSliceType>;
  MinMaxFilterType::Pointer minMaxFilter = MinMaxFilterType::New();
  minMaxFilter->SetInput(n_img);
  minMaxFilter->Update();
  // Get the maximum pixel value
  auto maxPixelValue = minMaxFilter->GetMaximum();
  // Print the maximum pixel value
  std::cout << "Maximum pixel value: " << maxPixelValue << std::endl;

  // flatfield
  kImageType::Pointer corrected_img = FlatfieldCorrection(img, dark, n_img, verbose);

  corrected_img->SetSpacing(img_spacing);

  // write file
  if (bit_depth == 8) {
    using PixelTypeOut = unsigned char;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(corrected_img, out_path, false, true, false);
  } else if (bit_depth == 16) {
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(corrected_img, out_path, false, true, false);
  } else if (bit_depth == 32) {
    using PixelTypeOut = float;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    WriteImageFile<kImageType,ImageTypeOut>(corrected_img, out_path, false, true, false);
  } else {
    std::cerr << "flatfield: unknown bit depth" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}