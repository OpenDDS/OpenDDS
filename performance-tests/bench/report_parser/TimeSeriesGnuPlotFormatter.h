#ifndef TIME_SERIES_GNUPLOT_FORMATTER_HEADER
#define TIME_SERIES_GNUPLOT_FORMATTER_HEADER

#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class TimeSeriesGnuPlotFormatter {
public:
  int format(const Report& report, std::ofstream& output_file_stream);
private:
  void output_header (const Report& report, std::ofstream& output_file_stream);
  void output_data(const Report& report, std::ofstream& output_file_stream);
};

#endif
