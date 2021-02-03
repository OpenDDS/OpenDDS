#include "ReportParser.h"
#include "TimeSeriesGnuPlotFormatter.h"
#include "TimeSeriesRawFormatter.h"

int ReportParser::parse(OutputType output_type, OutputFormat output_format,
    Report& report, std::ofstream& output_file_stream, ParseParameters& parseParameters)
{
  switch (output_type) {
  case OutputType::TimeSeries:
    return parse_time_series(output_format, report, output_file_stream,
      parseParameters);
    break;
  default:
    break;
  }

  return EXIT_FAILURE;
}

int ReportParser::parse_time_series(OutputFormat output_format, Report& report,
  std::ofstream& output_file_stream, ParseParameters& parseParameters)
{
  switch (output_format) {
  case OutputFormat::Gnuplot:
    TimeSeriesGnuPlotFormatter timeSeriesGnuPlotFormatter;
    return timeSeriesGnuPlotFormatter.format(report, output_file_stream,
      parseParameters);
    break;
  default:
    break;
  }

  TimeSeriesRawFormatter timeSeriesRawFormatter;
  return timeSeriesRawFormatter.format(report, output_file_stream);
}
