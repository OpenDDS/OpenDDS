#ifndef REPORT_PARSER_HEADER
#define REPORT_PARSER_HEADER

#include "Enums.h"
#include "ParseParameters.h"
#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class ReportParser {
public:
  int parse(OutputType output_type, OutputFormat output_format,
    Report& report, std::ofstream& output_file_stream, ParseParameters& parseParameters);
private:
  int parse_time_series(OutputFormat output_format, Report& report,
    std::ofstream& output_file_stream, ParseParameters& parseParameters);
};

#endif
