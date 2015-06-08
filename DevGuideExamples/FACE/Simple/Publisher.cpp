#include "FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Log_Msg.h"

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  // Create the pub connection
  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connId, dir, size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  // Message to send
  Messenger::Message msg = {"Hello, world.", 14, 1};

  // Send message
  FACE::TRANSACTION_ID_TYPE txn;
  ACE_DEBUG((LM_INFO, "Publisher: about to Send_Message()\n"));
  FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  // If not a reliable connection, must wait for message to hit the wire
  ACE_OS::sleep(15);

  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
