#ifndef RTPSRELAY_DOMAIN_STATISTICS_LISTENER_H_
#define RTPSRELAY_DOMAIN_STATISTICS_LISTENER_H_

#include "ListenerBase.h"

namespace RtpsRelay {

class DomainStatisticsListener : public ListenerBase {
public:

private:
  void on_data_available(DDS::DataReader_ptr reader) override;
};
}

#endif // RTPSRELAY_DOMAIN_STATISTICS_LISTENER_H_
