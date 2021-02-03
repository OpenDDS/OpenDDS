#ifndef ARGUMENT_PARSER_HEADER
#define ARGUMENT_PARSER_HEADER

#include "Enums.h"
#include "ParseParameters.h"
#include "BenchTypeSupportImpl.h"

#include <ace/ace_wchar.h> // For ACE_TCHAR

using namespace Bench::TestController;

class ArgumentParser {
public:
  bool parse(int argc, ACE_TCHAR* argv[], OutputType& output_type,
    OutputFormat& output_format, Report& report, std::ofstream& output_file_stream,
    ParseParameters& parseParameters);
private:
  void show_usage_prompt();
  void show_option_error(std::string option);
  void show_option_argument_error(std::string option_argument);
  void show_usage();
};

#endif
