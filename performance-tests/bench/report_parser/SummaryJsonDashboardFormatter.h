#ifndef SUMMARY_JSON_DASHBOARD_FORMATTER_HEADER
#define SUMMARY_JSON_DASHBOARD_FORMATTER_HEADER

#include "ParseParameters.h"

#include "BenchTypeSupportImpl.h"

namespace Bench {

class SummaryJsonDashboardFormatter {
public:
  int format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
};

} // namespace Bench

#endif
