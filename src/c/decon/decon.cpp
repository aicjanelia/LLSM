#include "decon.h"
#include "utils.h"
#include "defines.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
  // parameters
  bool overwrite = UNSET_BOOL;
  bool verbose = UNSET_BOOL;

  // declare the supported options
  po::options_description visible_opts("usage: decon [options] path\n\nAllowed options");
  visible_opts.add_options()
      ("help,h", "display this help message")
      ("output,o", po::value<std::string>()->required(),"output file path")
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
      std::cerr << "decon: deconvolves image with or without a PSF\n";
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

  // get output file path
  const char* out_path = varsmap["output"].as<std::string>().c_str();
  if (IsFile(out_path)) {
    if (!overwrite) {
      std::cerr << "decon: output path already exists" << std::endl;
      return EXIT_FAILURE;
    } else if (verbose) {
        std::cout << "overwriting: " << out_path << std::endl;
    }

  }

  // print parameters
  if (verbose) {
    std::cout << "\nInput Parameters\n";
    std::cout << "Input Path = " << in_path << "\n";
    std::cout << "Output Path = " << out_path << "\n";
    std::cout << "Overwrite = " << overwrite << "\n";
  }

  // decon

  return EXIT_SUCCESS;
}
