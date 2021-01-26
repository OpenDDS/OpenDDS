#ifndef TIME_SERIES_RAW_FORMATTER_HEADER
#define TIME_SERIES_RAW_FORMATTER_HEADER

#include "BenchTypeSupportImpl.h"

using namespace Bench::TestController;

class TimeSeriesRawFormatter {
public:
  int format(const Report& report, std::ofstream& output_file_stream);
};

#endif
