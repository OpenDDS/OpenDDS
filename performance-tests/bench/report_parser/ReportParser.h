#ifndef REPORT_PARSER_HEADER
#define REPORT_PARSER_HEADER

#include "Enums.h"
#include "ParseParameters.h"
#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class ReportParser {
public:
  int parse(const OutputType output_type, const OutputFormat output_format,
    const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
private:
  int parse_single_statistic(const OutputFormat output_format, const Report& report,
    std::ostream& output_stream, const ParseParameters& parse_parameters);
  int parse_time_series(const OutputFormat output_format, const Report& report,
    std::ostream& output_stream, const ParseParameters& parse_parameters);
};

#endif
