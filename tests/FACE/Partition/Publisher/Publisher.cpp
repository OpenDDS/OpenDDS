#include "Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdio.h"
#include "ace/Log_Msg.h"

#include <iostream>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::CONNECTION_ID_TYPE connId = 0;

  if (argc < 2) {
    std::cerr << "Publisher: requires number parameter" << argc << std::endl;
    status = FACE::INVALID_PARAM;
  } else {
    FACE::TS::Initialize("face_config.ini", status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

    int part = atoi(argv[1]);
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::MESSAGE_SIZE_TYPE size;
    char connection_name[16];
    ACE_OS::snprintf(connection_name, sizeof(connection_name), "pub_%d", part);
    FACE::TS::Create_Connection(connection_name, FACE::PUB_SUB, connId, dir, size,
                                FACE::INF_TIME_VALUE, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

    ACE_OS::sleep(5); // connection established with Subscriber

    ACE_DEBUG((LM_INFO, "(%P|%t) Publisher: about to send_message()\n"));
    for (FACE::Long i = 0; i < part; ++i) {
      if (i) ACE_OS::sleep(1);

      Messenger::Message msg = {"Hello, world.", part};
      FACE::TRANSACTION_ID_TYPE txn;
      ACE_DEBUG((LM_INFO, "(%P|%t) sending part: %d\n", part));
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
      if (status != FACE::RC_NO_ERROR) break;
    }
  }

  ACE_OS::sleep(10); // Subscriber receives message

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return static_cast<int>(status);
}
