#include "FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Log_Msg.h"

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
  FACE::TRANSACTION_ID_TYPE txn;
  Messenger::Message msg;

  ACE_DEBUG((LM_INFO, "Subscriber: about to Receive_Message()\n"));
  FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);

  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  ACE_DEBUG((LM_INFO, "%C\t%d\n", msg.from.in(), msg.count));

  FACE::CONNECTION_NAME_TYPE name = {};
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
  FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.LAST_MSG_VALIDITY != FACE::VALID) {
    ACE_ERROR((LM_ERROR,
               "ERROR: unexpected value in connection parameters after receiving\n"));
    return EXIT_FAILURE;
  }

  // Destroy the connection
  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
