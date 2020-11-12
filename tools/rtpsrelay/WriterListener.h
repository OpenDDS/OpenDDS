#ifndef RTPSRELAY_WRITER_LISTENER_H_
#define RTPSRELAY_WRITER_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "RelayHandler.h"
#include "DomainStatisticsReporter.h"

namespace RtpsRelay {

class WriterListener : public ListenerBase {
public:
  WriterListener(AssociationTable& association_table,
                 SpdpHandler& spdp_handler,
                 DomainStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  AssociationTable& association_table_;
  SpdpHandler& spdp_handler_;
  DomainStatisticsReporter& stats_reporter_;
};

}

#endif // RTPSRELAY_WRITER_LISTENER_H_
