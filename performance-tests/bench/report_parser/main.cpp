#include "ArgumentParser.h"
#include "ParseParameters.h"
#include "ReportParser.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  OutputType output_type = OutputType::None;
  OutputFormat output_format = OutputFormat::None;
  Bench::TestController::Report report{};
  std::shared_ptr<std::ostream> output_stream = NULL;
  Bench::ParseParameters parse_parameters;
  Bench::ArgumentParser argument_parser;

  if (!argument_parser.parse(argc, argv, output_type, output_format, report, output_stream, parse_parameters)) {
      return EXIT_FAILURE;
  }

  std::ostream& out = output_stream ? *output_stream : std::cout;

  Bench::ReportParser report_parser;
  return report_parser.parse(output_type, output_format, report, out, parse_parameters);
}
