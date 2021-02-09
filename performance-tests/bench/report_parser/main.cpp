#include "ArgumentParser.h"
#include "ParseParameters.h"
#include "ReportParser.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  OutputType output_type = OutputType::None;
  OutputFormat output_format = OutputFormat::None;
  Report report{};
  std::ofstream output_file_stream;
  ParseParameters parse_parameters;
  ArgumentParser argument_parser;

  if (!argument_parser.parse(argc, argv, output_type, output_format,
      report, output_file_stream, parse_parameters)) {
      return EXIT_FAILURE;
  }

  ReportParser report_parser;

  return report_parser.parse(output_type, output_format, report, output_file_stream,
    parse_parameters);
}
