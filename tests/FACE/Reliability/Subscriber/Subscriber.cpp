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
  long received = 0;

  if (!status) {
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size, status);
  }

  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    std::cout << "Subscriber: about to receive_message()" << std::endl;
    for (long i = 0; i < 100; ++i) {
      FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
      if (status != FACE::RC_NO_ERROR) break;
      std::cout << msg.text.in() << '\t' << msg.count << std::endl;
      if (msg.count != i) {
        std::cerr << "ERROR: Expected count " << i << ", got "
                  << msg.count << std::endl;
        status = FACE::INVALID_PARAM;
        break;
      } else {
        ++received;
      }
    }
  }

  if ((!status) && (received != 100)) {
    std::cerr << "ERROR: Expected 100 messages, got " << received << std::endl;
    status = FACE::INVALID_PARAM;
  }

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return static_cast<int>(status);
}
