#include "ArgumentParser.h"
#include "ParseParameters.h"
#include "ReportParser.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  OutputType output_type = OutputType::None;
  OutputFormat output_format = OutputFormat::None;
  Report report{};
  std::ofstream output_file_stream;
  ParseParameters parseParameters;
  ArgumentParser argumentParser;

  if (!argumentParser.parse(argc, argv, output_type, output_format,
      report, output_file_stream, parseParameters)) {
      return EXIT_FAILURE;
  }

  ReportParser reportParser;

  return reportParser.parse(output_type, output_format, report, output_file_stream,
    parseParameters);
}
