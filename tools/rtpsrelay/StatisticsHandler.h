#ifndef RTPSRELAY_STATISTICS_HANDLER_H_
#define RTPSRELAY_STATISTICS_HANDLER_H_

#include <ace/Event_Handler.h>

#include "RelayHandler.h"

class StatisticsHandler : public ACE_Event_Handler {
public:
  StatisticsHandler(ACE_Reactor * a_reactor,
                    RelayHandler * a_spdp_vertical, RelayHandler * a_spdp_horizontal,
                    RelayHandler * a_sedp_vertical, RelayHandler * a_sedp_horizontal,
                    RelayHandler * a_data_vertical, RelayHandler * a_data_horizontal);
  void open();
  int handle_timeout(ACE_Time_Value const & a_now, void const *) override;

private:
  RelayHandler * const m_spdp_vertical;
  RelayHandler * const m_spdp_horizontal;
  RelayHandler * const m_sedp_vertical;
  RelayHandler * const m_sedp_horizontal;
  RelayHandler * const m_data_vertical;
  RelayHandler * const m_data_horizontal;
  ACE_Time_Value m_last_collection;
};

#endif /* RTPSRELAY_STATISTICS_HANDLER_H_ */
