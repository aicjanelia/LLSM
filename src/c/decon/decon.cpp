#include "decon.h"
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "resampler.h"
#include "writer.h"
#include "z_block.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>
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
  unsigned int threadnum = UNSET_UNSIGNED_INT;
  std::uint64_t block_depth = 0;
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
      ("thread,t", po::value<unsigned int>(&threadnum)->default_value(1),"number of threads")
      ("block-depth", po::value<std::uint64_t>(&block_depth)->default_value(0),"Z core depth per block (0 selects 512, reduced automatically when required)")
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

    // set thread number
    itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(threadnum);

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
  const std::string in_path = varsmap["input"].as<std::string>();
  if (!IsFile(in_path.c_str())) {
    std::cerr << "decon: input path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const std::string kernel_path = varsmap["kernel"].as<std::string>();
  if (!IsFile(kernel_path.c_str())) {
    std::cerr << "decon: kernel path is not a file" << std::endl;
    return EXIT_FAILURE;
  }
  const std::string out_path = varsmap["output"].as<std::string>();
  if (IsFile(out_path.c_str())) {
    if (fs::equivalent(in_path, out_path)) {
      std::cerr << "decon: input and output paths must be different for block processing" << std::endl;
      return EXIT_FAILURE;
    }
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
    std::cout << "Threads = " << threadnum << "\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Kernel Path = " << kernel_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
    std::cout << "Bit Depth = " << bit_depth << "\n";
    std::cout << "Requested Block Depth = " << block_depth << std::endl;
  }

  // Start timing
  auto start_time = std::chrono::high_resolution_clock::now();

  try {
    TiffStackBlockReader input_reader(in_path);
    kImageType::Pointer kernel = ReadImageFile<kImageType>(kernel_path);
    if (kernel == nullptr) {
      std::cerr << "decon: unable to read kernel image" << std::endl;
      return EXIT_FAILURE;
    }

    kImageType::SpacingType img_spacing = input_reader.GetSpacing();
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
    kernel->SetSpacing(kernel_spacing);

    if (img_spacing[2] != kernel_spacing[2])
    {
      kernel = Resampler(kernel, img_spacing, verbose);
    }

    const kImageType::SizeType kernel_size =
        kernel->GetLargestPossibleRegion().GetSize();
    const std::uint64_t halo_depth =
        llsm::RequiredHaloDepth(kernel_size[2], iterations);
    const std::uint64_t maximum_input_depth = llsm::MaximumInputDepthForFFT(
        input_reader.GetWidth(), input_reader.GetHeight(),
        kernel_size[0], kernel_size[1], kernel_size[2]);
    const std::uint64_t selected_core_depth = llsm::SelectCoreDepth(
        input_reader.GetDepth(), block_depth, halo_depth, maximum_input_depth);
    const std::vector<llsm::ZBlock> blocks = llsm::MakeZBlocks(
        input_reader.GetDepth(), selected_core_depth, halo_depth);

    if (verbose) {
      std::cout << "Input Dimensions = " << input_reader.GetWidth() << " "
                << input_reader.GetHeight() << " " << input_reader.GetDepth() << "\n";
      std::cout << "Resampled Kernel Dimensions = " << kernel_size[0] << " "
                << kernel_size[1] << " " << kernel_size[2] << "\n";
      std::cout << "Z Halo = " << halo_depth << "\n";
      std::cout << "Selected Core Depth = " << selected_core_depth << "\n";
      std::cout << "Number of Z Blocks = " << blocks.size() << std::endl;
    }

    kImageType::SpacingType output_spacing;
    output_spacing.Fill(1.0);
    TiffStackBlockWriter output_writer(
        out_path, input_reader.GetWidth(), input_reader.GetHeight(),
        input_reader.GetDepth(), bit_depth, output_spacing);

    const kPixelType normalized_subtract =
        static_cast<kPixelType>(subtract_constant) /
        std::numeric_limits<unsigned short>::max();

    for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index)
    {
      const llsm::ZBlock &block = blocks[block_index];
      if (verbose) {
        std::cout << "Processing Z block " << block_index + 1 << "/" << blocks.size()
                  << ": core [" << block.core_begin << ", "
                  << block.core_begin + block.core_size << "), input ["
                  << block.read_begin << ", " << block.read_begin + block.read_size
                  << ")" << std::endl;
      }

      kImageType::Pointer image_block = input_reader.ReadBlock(
          block.read_begin, block.read_size, img_spacing, normalized_subtract);
      kImageType::Pointer decon_block =
          RichardsonLucy(image_block, kernel, iterations, verbose);
      output_writer.AppendCore(decon_block, block.core_offset, block.core_size);
    }
    output_writer.Finish();
  } catch (const itk::ExceptionObject& e) {
    std::cerr << "decon: ITK error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // End timing and display results
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  
  std::cout << "\n=== Processing Complete ===\n";
  std::cout << "Threads used: " << threadnum << "\n";
  std::cout << "Processing time: " << duration.count() / 1000.0 << " seconds" << std::endl;

  return EXIT_SUCCESS;
}
