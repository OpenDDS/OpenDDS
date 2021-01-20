#include <util.h>
#include <json_conversion.h>
#include <PropertyStatBlock.h>

#include "BenchTypeSupportImpl.h"

#include <iostream>
#include <fstream>
#include <util.h>

#include <dds/DCPS/RapidJsonWrapper.h>

#include <ace/ace_wchar.h> // For ACE_TCHAR
#include <ace/OS.h> // for ACE_OS

using namespace Bench;
using namespace Bench::TestController;

Report report{};
std::ofstream outputFileStream;

bool parse_arguments(int argc, ACE_TCHAR* argv[]) {
  const char* usage = "usage: report_parser [-h|--help] | [OPTIONS...]";
  std::string input_file_path;
  std::string output_file_path;

  if (argc == 1) {
    std::cout << usage << std::endl
      << "Use -h or --help to see usage options" << std::endl;
  }
  else {
    for (int i = 1; i < argc; i++) {
      const ACE_TCHAR* argument = argv[i];
      if (!ACE_OS::strcmp(argument, ACE_TEXT("--help")) || !ACE_OS::strcmp(argument, ACE_TEXT("-h"))) {
        std::cout << usage << std::endl
          << std::endl
          << "OPTIONS:" << std::endl
          << "--time-series                Parses time-series data." << std::endl
          << "--input-file <filename>      The report file to read" << std::endl
          << "--output-file <filename>     The parsed file to generate" << std::endl;
        return false;
      }
      else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--input-file"))) {
        input_file_path = get_option_argument(i, argc, argv);
      }
      else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--output-file"))) {
        output_file_path = get_option_argument(i, argc, argv);
      }
      else {
        std::cerr << "Invalid option: " << argument << std::endl;
        std::cerr << usage << std::endl
          << "Use -h or --help to see the full help message" << std::endl;
        return false;
      }
    }
  }

  if (input_file_path.empty()) {
    std::cerr << "Input file not specified" << std::endl;
    return false;
  }
  else if (output_file_path.empty()) {
    std::cerr << "Output file not specified" << std::endl;
    return false;
  }
  else {
    std::ifstream ifs(input_file_path);
    if (ifs.is_open()) {
      if (!json_2_idl(ifs, report)) {
        std::cerr << "Could not parse " << input_file_path << std::endl;
        return false;
      }
      else {
        outputFileStream.open(output_file_path);
        if (!outputFileStream.is_open()) {
          std::cerr << "Could not open output file " << output_file_path << std::endl;
          return false;
        }
      }
    }
    else {
      std::cerr << "Could not open input file " << input_file_path << std::endl;
      return false;
    }
  }

  return true;
}

int parse_report() {
  /* Functionality will be added later. */
  return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::cout << "NOT YET IMPLEMENTED!\n";

  if (!parse_arguments(argc, argv)) {
    return EXIT_FAILURE;
  }

  if (!parse_report()) {
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
