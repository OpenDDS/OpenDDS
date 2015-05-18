#include "Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include <iostream>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status = FACE::RC_NO_ERROR;
  FACE::TS::Initialize("face_config.ini", status);
  FACE::CONNECTION_ID_TYPE connId;
  FACE::MESSAGE_SIZE_TYPE size;

  FACE::CONNECTION_ID_TYPE connId_INF;
  FACE::MESSAGE_SIZE_TYPE size_INF;
  FACE::CONNECTION_DIRECTION_TYPE dir_INF;

  if (!status) {
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size,
                                FACE::INF_TIME_VALUE, status);
    FACE::TS::Create_Connection("sub_INF", FACE::PUB_SUB, connId_INF, dir_INF,
                                size_INF, FACE::INF_TIME_VALUE, status);
  }

  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    long expected = 0;
    std::cout << "Subscriber: about to receive_message()" << std::endl;
    while (expected <= 19) {
      FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
      if (status != FACE::RC_NO_ERROR) break;
      std::cout << msg.text.in() << '\t' << msg.count << std::endl;
      if ((msg.count != expected) && expected > 0) {
        std::cerr << "ERROR: Expected count " << expected << ", got "
                  << msg.count << std::endl;
        status = FACE::INVALID_PARAM;
        break;
      } else {
        expected = msg.count + 1;
      }
    }
  }

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status_INF = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId_INF, destroy_status_INF);
  if ((destroy_status_INF != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status_INF;
  }

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  if (status) {
    std::cout << "Subscriber status " << status << std::endl;
  }
  return static_cast<int>(status);
}
