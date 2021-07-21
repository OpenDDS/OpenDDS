#ifndef ARGUMENT_PARSER_HEADER
#define ARGUMENT_PARSER_HEADER

#include "Enums.h"
#include "ParseParameters.h"
#include "BenchTypeSupportImpl.h"

#include <ace/ace_wchar.h> // For ACE_TCHAR

namespace Bench {

class ArgumentParser {
public:
  bool parse(int argc, ACE_TCHAR* argv[], OutputType& output_type,
    OutputFormat& output_format, Bench::TestController::Report& report, std::shared_ptr<std::ostream>& output_stream,
    ParseParameters& parse_parameters);
private:
  void show_usage_prompt();
  void show_option_error(std::string option);
  void show_option_argument_error(std::string option_argument);
  void show_usage();
  void check_for_iperf(Bench::TestController::Report& report);
};

} // namespace Bench

#endif
