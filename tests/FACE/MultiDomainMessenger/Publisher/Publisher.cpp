#include "../Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/Log_Msg.h"

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_NAME_TYPE name1 = {};
  FACE::CONNECTION_ID_TYPE connId1;
  FACE::CONNECTION_DIRECTION_TYPE dir1;
  FACE::MESSAGE_SIZE_TYPE size1;
  FACE::TS::Create_Connection("pub1", FACE::PUB_SUB, connId1, dir1, size1,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_NAME_TYPE name2 = {};
  FACE::CONNECTION_ID_TYPE connId2;
  FACE::CONNECTION_DIRECTION_TYPE dir2;
  FACE::MESSAGE_SIZE_TYPE size2;
  FACE::TS::Create_Connection("pub2", FACE::PUB_SUB, connId2, dir2, size2,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;

  ACE_OS::sleep(5); // connection established with Subscriber

  Messenger::Message msg = {"Hello, world.", 0, 0};

#ifdef ACE_HAS_CDR_FIXED
  msg.deci = FACE::Fixed("987.654");
#endif

  FACE::TRANSACTION_ID_TYPE txn;
  ACE_DEBUG((LM_INFO, "Publisher1: about to Send_Message()\n"));
  FACE::TS::Send_Message(connId1, FACE::INF_TIME_VALUE, txn, msg, size1, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  ACE_DEBUG((LM_INFO, "Publisher2: about to Send_Message()\n"));
  FACE::TS::Send_Message(connId2, FACE::INF_TIME_VALUE, txn, msg, size2, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::TS::Get_Connection_Parameters(name1, connId1, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.MESSAGE != 0
      || connectionStatus.LAST_MSG_VALIDITY != FACE::VALID
      || connectionStatus.WAITING_PROCESSES_OR_MESSAGES != 0
      || connectionStatus.CONNECTION_DIRECTION != FACE::SOURCE
      || connectionStatus.REFRESH_PERIOD != 0) {
    ACE_ERROR((LM_ERROR, "ERROR: unexpected value in connection "
               "parameters after sending\n"));
    return EXIT_FAILURE;
  }

  FACE::TS::Get_Connection_Parameters(name2, connId2, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.MESSAGE != 0
      || connectionStatus.LAST_MSG_VALIDITY != FACE::VALID
      || connectionStatus.WAITING_PROCESSES_OR_MESSAGES != 0
      || connectionStatus.CONNECTION_DIRECTION != FACE::SOURCE
      || connectionStatus.REFRESH_PERIOD != 0) {
    ACE_ERROR((LM_ERROR, "ERROR: unexpected value in connection "
               "parameters after sending\n"));
    return EXIT_FAILURE;
  }

  ACE_OS::sleep(15); // Subscriber receives message

  FACE::TS::Destroy_Connection(connId1, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::TS::Destroy_Connection(connId2, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
