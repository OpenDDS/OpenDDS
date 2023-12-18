#ifndef RTPSRELAY_STATISTICS_WRITER_LISTENER_H_
#define RTPSRELAY_STATISTICS_WRITER_LISTENER_H_

#include "RelayStatisticsReporter.h"
#include "WriterListenerBase.h"

namespace RtpsRelay {

class StatisticsWriterListener: public WriterListenerBase {
public:
  using ReporterMemFun = void (RelayStatisticsReporter::*)(uint32_t, const OpenDDS::DCPS::MonotonicTimePoint&);

  StatisticsWriterListener(RelayStatisticsReporter& relay_statistics_reporter, ReporterMemFun update)
    : relay_statistics_reporter_(relay_statistics_reporter)
    , update_(update)
  {}

  void on_publication_matched(DDS::DataWriter_ptr /*writer*/,
                              const DDS::PublicationMatchedStatus & status) override
  {
    (relay_statistics_reporter_.*update_)(status.current_count, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

private:
  RelayStatisticsReporter& relay_statistics_reporter_;
  ReporterMemFun update_;
};

}

#endif // RTPSRELAY_STATISTICS_WRITER_LISTENER_H_
