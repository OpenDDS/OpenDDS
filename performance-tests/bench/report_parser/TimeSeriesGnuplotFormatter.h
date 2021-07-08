#ifndef TIME_SERIES_GNUPLOT_FORMATTER_HEADER
#define TIME_SERIES_GNUPLOT_FORMATTER_HEADER

#include "ParseParameters.h"

#include "BenchTypeSupportImpl.h"

namespace Bench {

class TimeSeriesGnuplotFormatter {
public:
  int format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
private:
  void output_header(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
  void output_data(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
};

} // namespace Bench

#endif
