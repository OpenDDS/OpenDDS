#ifndef TIME_SERIES_GNUPLOT_FORMATTER_HEADER
#define TIME_SERIES_GNUPLOT_FORMATTER_HEADER

#include "ParseParameters.h"

#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

// TODO RENAME 'Gnuplot'
class TimeSeriesGnuPlotFormatter {
public:
  int format(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
private:
  void output_header(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
  void output_data(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters);
};

#endif
