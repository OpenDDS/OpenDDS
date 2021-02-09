#ifndef REPORT_PARSER_HEADER
#define REPORT_PARSER_HEADER

#include "Enums.h"
#include "ParseParameters.h"
#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class ReportParser {
public:
  int parse(const OutputType output_type, const OutputFormat output_format,
    const Report& report, std::ofstream& output_file_stream, const ParseParameters& parse_parameters);
private:
  int parse_time_series(const OutputFormat output_format, const Report& report,
    std::ofstream& output_file_stream, const ParseParameters& parse_parameters);
};

#endif
