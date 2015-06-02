#include "Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_time.h"
#include <iostream>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connId, dir, size, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  ACE_OS::sleep(10); // connection established with Subscriber

  std::cout << "Publisher: about to send_message()" << std::endl;
  for (FACE::Long i = 0; i < 20; ++i) {
    Messenger::Message msg = {"Hello, world.", i, 0};
    FACE::TRANSACTION_ID_TYPE txn;
    std::cout << "  sending " << i << std::endl;
    int retries = 40;
    do {
      if (status == FACE::TIMED_OUT) {
        if (retries % 5 == 0) {
          std::cout << "Send_Message timed out (x5), keep trying, resending msg " << i << std::endl;
        }
        --retries;
      }
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
    } while (status == FACE::TIMED_OUT && retries > 0);

    if (status != FACE::RC_NO_ERROR) break;
  }

  ACE_OS::sleep(15); // Subscriber receives messages

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return static_cast<int>(status);
}
