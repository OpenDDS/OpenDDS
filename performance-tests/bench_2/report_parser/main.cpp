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

int parse_report(Report report, std::ofstream& ofs) {
  /* Functionality will be added later. */
  return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  /* Functionality will be added later for parsing the output of test_controller. */
  std::cout << "NOT YET IMPLEMENTED!\n";

  int result = EXIT_SUCCESS;
  
  const char* usage = "usage: report_parser [-h|--help] | [OPTIONS...]";
  std::string input_file_path;
  std::string output_file_path;

  try {
    if (argc == 1) {
      std::cout << usage << std::endl
        << "Use -h or --help to see usage options" << std::endl;
    } else {
      for (int i = 1; i < argc; i++) {
        const ACE_TCHAR* argument = argv[i];
        if (!ACE_OS::strcmp(argument, ACE_TEXT("--help")) || !ACE_OS::strcmp(argument, ACE_TEXT("-h"))) {
          std::cout << usage << std::endl
            << std::endl
            << "OPTIONS:" << std::endl
            << "--time-series                Parses time-series data." << std::endl
            << "--input-file <filename>      The report file to read" << std::endl
            << "--output-file <filename>     The parsed file to generate" << std::endl;
          return 0;
        } else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--input-file"))) {
          input_file_path = get_option_argument(i, argc, argv);
        } else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--output-file"))) {
          output_file_path = get_option_argument(i, argc, argv);
        } else {
          std::cerr << "Invalid option: " << argument << std::endl;
          throw EXIT_FAILURE;
        }
      }
    }
  } catch (int value) {
    std::cerr << usage << std::endl
      << "Use -h or --help to see the full help message" << std::endl;
    return value;
  }

  if (input_file_path.empty()) {
    std::cerr << "Input file not specified" << std::endl;
    result = EXIT_FAILURE;
  } else if (output_file_path.empty()) {
    std::cerr << "Output file not specified" << std::endl;
    result = EXIT_FAILURE;
  } else {
    std::ifstream ifs(input_file_path);
    if (ifs.is_open()) {
      Report report{};
      if (!json_2_idl(ifs, report)) {
        std::cerr << "Could not parse " << input_file_path << std::endl;
        return EXIT_FAILURE;
      } else {
        std::ofstream ofs(output_file_path);
        if (ofs.is_open()) {
          result = parse_report(report, ofs);
          ofs.close();
        } else {
          std::cerr << "Could not open output file " << output_file_path << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
    else {
      std::cerr << "Could not open input file " << input_file_path << std::endl;
      return EXIT_FAILURE;
    }
  }

  return result;
}
