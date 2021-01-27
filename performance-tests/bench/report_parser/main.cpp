#include "ArgumentParser.h"
#include "TimeSeriesGnuPlotFormatter.h"
#include "TimeSeriesRawFormatter.h"

int parse_time_series(OutputFormat output_format, Report& report,
  std::ofstream& output_file_stream) {
  switch (output_format) {
  case OutputFormat::Gnuplot:
    TimeSeriesGnuPlotFormatter timeSeriesGnuPlotFormatter;
    return timeSeriesGnuPlotFormatter.format(report, output_file_stream);
    break;
  default:
    break;
  }

  TimeSeriesRawFormatter timeSeriesRawFormatter;
  return timeSeriesRawFormatter.format(report, output_file_stream);
}

int parse_report(OutputType output_type, OutputFormat output_format,
  Report& report, std::ofstream& output_file_stream) {
  switch (output_type) {
  case OutputType::TimeSeries:
    return parse_time_series(output_format, report, output_file_stream);
    break;
  default:
    break;
  }

  return EXIT_FAILURE;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  OutputType output_type = OutputType::None;
  OutputFormat output_format = OutputFormat::None;
  Report report{};
  std::ofstream output_file_stream;

  ArgumentParser argumentParser;

  if (!argumentParser.parse(argc, argv, output_type, output_format,
    report, output_file_stream)) {
    return EXIT_FAILURE;
  }

  if (!parse_report(output_type, output_format, report, output_file_stream)) {
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
