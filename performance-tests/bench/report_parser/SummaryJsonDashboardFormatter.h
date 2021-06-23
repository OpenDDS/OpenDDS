#ifndef SUMMARY_JSON_DASHBOARD_FORMATTER_HEADER
#define SUMMARY_JSON_DASHBOARD_FORMATTER_HEADER

#include "ParseParameters.h"

#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class SummaryJsonDashboardFormatter {
public:
  int format(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
};

#endif
