#include "FaceMessage_TS.hpp"
#include "ace/Log_Msg.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

// FUZZ: disable check_for_improper_main_declaration
int main(int, char*[])
{
  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Create the sub connection
  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;
  FACE::TS::Create_Connection(
    "sub", FACE::PUB_SUB, connId, dir,
    max_msg_size, FACE::INF_TIME_VALUE, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Receive a message
  const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
  FACE::TRANSACTION_ID_TYPE txn;
  Messenger::Message msg;

  ACE_DEBUG((
    LM_INFO, "Subscriber: about to Receive_Message()\n"));

  FACE::TS::Receive_Message(
    connId, timeout, txn, msg, max_msg_size, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Output the message
  ACE_DEBUG((LM_INFO, "%C\t%d\n", msg.text.in(), msg.count));

  // Destroy the sub connection
  FACE::TS::Destroy_Connection(connId, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  return EXIT_SUCCESS;
}
// FUZZ: enable check_for_improper_main_declaration
