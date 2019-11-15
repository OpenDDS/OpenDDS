#ifndef RTPSRELAY_STATISTICS_HANDLER_H_
#define RTPSRELAY_STATISTICS_HANDLER_H_

#include "RelayHandler.h"

#include <dds/DCPS/TimeTypes.h>

#include <ace/Event_Handler.h>

namespace RtpsRelay {

class StatisticsHandler : public ACE_Event_Handler {
public:
  StatisticsHandler(ACE_Reactor* a_reactor,
                    RelayHandler* a_spdp_vertical, RelayHandler* a_spdp_horizontal,
                    RelayHandler* a_sedp_vertical, RelayHandler* a_sedp_horizontal,
                    RelayHandler* a_data_vertical, RelayHandler* a_data_horizontal);
  void open();

private:
  int handle_timeout(const ACE_Time_Value& a_now, const void*) override;

  RelayHandler* const spdp_vertical_;
  RelayHandler* const spdp_horizontal_;
  RelayHandler* const sedp_vertical_;
  RelayHandler* const sedp_horizontal_;
  RelayHandler* const data_vertical_;
  RelayHandler* const data_horizontal_;
  OpenDDS::DCPS::MonotonicTimePoint m_last_collection;
};

}

#endif /* RTPSRELAY_STATISTICS_HANDLER_H_ */
