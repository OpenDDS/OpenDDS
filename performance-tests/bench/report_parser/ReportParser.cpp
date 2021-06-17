#include "ReportParser.h"

#include "StatBlockRawFormatter.h"

#include "TimeSeriesGnuPlotFormatter.h"
#include "TimeSeriesRawFormatter.h"

int ReportParser::parse(const OutputType output_type, const OutputFormat output_format,
    const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  switch (output_type) {
  case OutputType::SingleStatistic:
    return parse_single_statistic(output_format, report, output_stream, parse_parameters);
    break;
  case OutputType::TimeSeries:
    return parse_time_series(output_format, report, output_stream, parse_parameters);
    break;
  default:
    break;
  }

  return EXIT_FAILURE;
}

int ReportParser::parse_single_statistic(const OutputFormat output_format, const Report& report,
    std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  switch (output_format) {
  case OutputFormat::StatBlock:
    StatBlockRawFormatter stat_block_raw_formatter;
    return stat_block_raw_formatter.format(report, output_stream, parse_parameters);
    break;
  default:
    break;
  }

  return EXIT_FAILURE;
}

int ReportParser::parse_time_series(const OutputFormat output_format, const Report& report,
    std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  switch (output_format) {
  case OutputFormat::Gnuplot:
    TimeSeriesGnuPlotFormatter time_series_gnuplot_formater;
    return time_series_gnuplot_formater.format(report, output_stream, parse_parameters);
    break;
  default:
    break;
  }

  TimeSeriesRawFormatter time_series_raw_formater;
  return time_series_raw_formater.format(report, output_stream, parse_parameters);
}
