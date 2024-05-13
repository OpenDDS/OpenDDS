#include "StockQuoterC.h"
#include "StockQuoter_TS.hpp"
#include "ace/Log_Msg.h"
#include <FACE/TS_common.hpp>
#include <FACE/common.hpp>
#include <csignal>

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

// FUZZ: disable check_for_improper_main_declaration
int main(int, char *[]) {

  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO, "Subscriber: error initializing FACE, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  // Create the sub connection
  FACE::CONNECTION_ID_TYPE connIdQuote;
  FACE::CONNECTION_ID_TYPE connIdExchange;
  FACE::CONNECTION_DIRECTION_TYPE dir = FACE::DESTINATION;
  FACE::CONNECTION_DIRECTION_TYPE dirQuote = FACE::DESTINATION;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;
  FACE::TS::Create_Connection("subquote", FACE::PUB_SUB, connIdQuote, dirQuote,
                              max_msg_size, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO, "Subscriber: no error\n", status));
  }

  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connIdExchange, dir,
                              max_msg_size, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO, "Subscriber: error creating connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  // Receive a message
  while (true) {
    ACE_OS::sleep(1);

    ACE_DEBUG((LM_INFO, "Subscriber: waiting for message\n"));

    StockQuoter::Quote msg;
    FACE::TRANSACTION_ID_TYPE txnExchange;
    FACE::TRANSACTION_ID_TYPE txnQuote;

    StockQuoter::ExchangeEvent event;
    FACE::TS::Receive_Message(connIdExchange, FACE::INF_TIME_VALUE, txnExchange,
                              event, max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) {
      ACE_DEBUG((LM_INFO,
                 "Subscriber: error receiving EVENT message, status = %d\n",
                 status));
    }

    ACE_OS::sleep(2);

    FACE::TS::Receive_Message(connIdQuote, FACE::INF_TIME_VALUE, txnQuote, msg,
                              max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) {
      ACE_DEBUG((LM_INFO,
                 "Subscriber: error receiving QUOTE message, status = %d\n",
                 status));
      return static_cast<int>(status);
    }

    // print all the fields of msg
    ACE_DEBUG((LM_INFO, "Subscriber: received message\n"));
    ACE_DEBUG((LM_INFO, "  exchange: %s\n", msg.exchange.in()));
    ACE_DEBUG((LM_INFO, "  value: %f\n", msg.value));
  }

  // Destroy the sub connection
  FACE::TS::Destroy_Connection(connIdQuote, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO,
               "Subscriber: error destroying connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  FACE::TS::Destroy_Connection(connIdExchange, status);
  if (status != FACE::RC_NO_ERROR) {
    ACE_DEBUG((LM_INFO,
               "Subscriber: error destroying connections, status = %d\n",
               status));
    return static_cast<int>(status);
  }

  return EXIT_SUCCESS;
}
