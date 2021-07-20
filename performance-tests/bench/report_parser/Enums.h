#ifndef REPORT_PARSER_ENUMS_HEADER
#define REPORT_PARSER_ENUMS_HEADER

enum class OutputType
{
  None,
  SingleStatistic,
  Summary,
  TimeSeries
};

enum class OutputFormat
{
  None,
  StatBlock,
  Gnuplot,
  Json
};

#endif
