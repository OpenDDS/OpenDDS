
#include "StockQuoterC.h"
#include "StockQuoter_TS.hpp"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_unistd.h"
#include "orbsvcs/Time_Utilities.h"
#include <FACE/TS.hpp>
#include <FACE/TS_common.hpp>
#include <ace/OS_NS_time.h>

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

TimeBase::TimeT get_timestamp() {
  TimeBase::TimeT retval;
  ACE_timer_t t = ACE_OS::gethrtime();
  ORBSVCS_Time::hrtime_to_TimeT(retval, t);
  return retval;
}

// FUZZ: disable check_for_improper_main_declaration
int main(int, char *[]) {
  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status, status2;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG(
        (LM_INFO, "Publisher: error initializing FACE, status = %d\n", status));
    return static_cast<int>(status);
  }

  // Create the pub Exchangeconnection
  FACE::CONNECTION_ID_TYPE connIdExchange;
  FACE::CONNECTION_DIRECTION_TYPE dir = FACE::SOURCE;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;

  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connIdExchange, dir,
                              max_msg_size, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO, "Publisher: error creating connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  FACE::CONNECTION_ID_TYPE connIdQuote;
  FACE::CONNECTION_DIRECTION_TYPE dirQuote = FACE::SOURCE;
  FACE::TS::Create_Connection("pubquote", FACE::PUB_SUB, connIdQuote, dirQuote,
                              max_msg_size, FACE::INF_TIME_VALUE, status2);
  if (status2 != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO, "Publisher: error creating connections, status = %d\n ",
               status));
    return static_cast<int>(status2);
  }

  // Message to send
  for (size_t i = 0; i < 10; i++) {
    ACE_OS::sleep(1);

    StockQuoter::Quote msg;
    msg.ticker = "AAPL";
    msg.exchange = "NASDAQ";
    msg.full_name = "Apple Inc.";
    msg.value = 100.0 + i;
    msg.timestamp = get_timestamp();
    FACE::TRANSACTION_ID_TYPE txnQuote;
    FACE::TRANSACTION_ID_TYPE txnExchange;

    StockQuoter::ExchangeEvent event;
    event.exchange = "NASDAQ";
    event.event = StockQuoter::TRADING_OPENED;
    event.timestamp = get_timestamp();

    // Send the event
    FACE::TS::Send_Message(connIdExchange, FACE::INF_TIME_VALUE, txnExchange,
                           event, max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) {
      ACE_DEBUG((LM_INFO,
                 "Publisher: error sending EVENT message, status = %d\n",
                 status));
      return static_cast<int>(status);
    }

    ACE_OS::sleep(1);

    // Send the message
    FACE::TS::Send_Message(connIdQuote, FACE::INF_TIME_VALUE, txnQuote, msg,
                           max_msg_size, status2);
    if (status != FACE::RC_NO_ERROR) {
      ACE_DEBUG((LM_INFO,
                 "Publisher: error sending QUOTE message, status = %d\n",
                 status));
      return static_cast<int>(status);
    }

    ACE_DEBUG((LM_INFO, "Publisher: sent message\n"));
  }

  // Give message time to be processed before exiting
  ACE_OS::sleep(15);

  // Destroy the pub connection
  FACE::TS::Destroy_Connection(connIdExchange, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO,
               "Publisher: error destroying connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  FACE::TS::Destroy_Connection(connIdQuote, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO,
               "Publisher: error destroying connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  return EXIT_SUCCESS;
}
// FUZZ: enable check_for_improper_main_declaration
