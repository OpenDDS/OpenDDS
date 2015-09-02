#include "Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_stdio.h"
#include "ace/Log_Msg.h"

#include <iostream>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  FACE::RETURN_CODE_TYPE status = FACE::RC_NO_ERROR;
  FACE::CONNECTION_ID_TYPE connId = 0;
  FACE::MESSAGE_SIZE_TYPE size;
  int part = atoi(argv[1]);
  bool partitions_received[] = { false, false };

  if (argc < 2) {
    std::cerr << "Subscriber: requires number parameter" << argc << std::endl;
    status = FACE::INVALID_PARAM;
  } else {
    FACE::TS::Initialize("face_config.ini", status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
  }

  if (!status) {
    FACE::CONNECTION_DIRECTION_TYPE dir;
    char connection_name[16];
    ACE_OS::snprintf(connection_name, sizeof(connection_name), "sub_%d", part);

    FACE::TS::Create_Connection(connection_name, FACE::PUB_SUB, connId, dir, size,
                                FACE::INF_TIME_VALUE, status);
  }

  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    for (long i = 0; i < part; ++i) {
      ACE_DEBUG((LM_INFO, "(%P|%t) Subscriber: about to receive_message()\n"));
      FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
      if (status != FACE::RC_NO_ERROR) break;
      ACE_DEBUG((LM_INFO, "(%P|%t) Subscriber%d: %s part: %d\n",
                 part, msg.text.in(), msg.count));
      partitions_received[msg.count - 1] = true;
    }
    if (status) {
      std::cout << "Subscriber: receive_message() status " << status << std::endl;
    }
  }

  // Check received
  if (!status) {
    if (part != 2) {
      // Should receive 1
      if (!partitions_received[0]) {
        std::cerr << "ERROR: Expected to receive on partition 1" << std::endl;
        status = FACE::INVALID_PARAM;
      }
    }
    if (part != 1) {
      // Should receive 2
      if (!partitions_received[1]) {
        std::cerr << "ERROR: Expected to receive on partition 2" << std::endl;
        status = FACE::INVALID_PARAM;
      }
    }
  }

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    std::cerr << "ERROR: destroying connection" << std::endl;
    status = destroy_status;
  }

  return static_cast<int>(status);
}
