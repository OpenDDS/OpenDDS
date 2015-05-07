#include "../Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include <iostream>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connId, dir, size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_NAME_TYPE name = {};
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
  FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  //Check that name & connection validated ok
  FACE::CONNECTION_NAME_TYPE goodName = "pub";
  FACE::TS::Get_Connection_Parameters(goodName, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) {
    std::cout << "ERROR: Get_Connection_Parameters with goodName and good connection Id failed\n";
  }

  //Check that wrong name & connection validated - INVALID_PARAM
  FACE::CONNECTION_NAME_TYPE badName = "wrong_pub";
  FACE::TS::Get_Connection_Parameters(badName, connId, connectionStatus, status);
  if (status != FACE::INVALID_PARAM) {
    std::cout << "ERROR: Get_Connection_Parameters with bad name and good connection Id failed to return INVALID_PARAM\n";
  }

  //Check that name & wrong connection validated - INVALID_PARAM
  FACE::CONNECTION_ID_TYPE wrongConnectionId = 5;
  FACE::TS::Get_Connection_Parameters(goodName, wrongConnectionId, connectionStatus, status);
  if (status != FACE::INVALID_PARAM) {
    std::cout << "ERROR: Get_Connection_Parameters with good name and bad connection Id failed to return INVALID_PARAM\n";
  }

  //Check that no name & no connection validated - INVALID_PARAM
  FACE::CONNECTION_NAME_TYPE noName = {};
  FACE::CONNECTION_ID_TYPE noConnectionId = 0;
  FACE::TS::Get_Connection_Parameters(noName, noConnectionId, connectionStatus, status);
  if (status != FACE::INVALID_PARAM) {
    std::cout << "ERROR: Get_Connection_Parameters with no name and no connection Id failed to return INVALID_PARAM\n";
  }
  FACE::CONNECTION_ID_TYPE getConnectionId = 0;
  FACE::TS::Get_Connection_Parameters(name, getConnectionId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.LAST_MSG_VALIDITY != FACE::INVALID
      || connectionStatus.CONNECTION_DIRECTION != FACE::SOURCE
      || getConnectionId != connId) {
    std::cout << "ERROR: unexpected value in connection parameters before sending\n";
    return EXIT_FAILURE;
  }

  ACE_OS::sleep(5); // connection established with Subscriber

  Messenger::Message msg = {"Hello, world.", 0, 0};

#ifdef ACE_HAS_CDR_FIXED
  msg.deci = FACE::Fixed("987.654");
#endif

  FACE::TRANSACTION_ID_TYPE txn;
  std::cout << "Publisher: about to Send_Message()" << std::endl;
  FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.MESSAGE != 1
      || connectionStatus.LAST_MSG_VALIDITY != FACE::VALID) {
    std::cout << "ERROR: unexpected value in connection parameters after sending\n";
    return EXIT_FAILURE;
  }

  ACE_OS::sleep(15); // Subscriber receives message

  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
