#include "Idl/FaceMessage_TS.hpp"

#include "ace/OS_NS_unistd.h"
#include <iostream>

int main(int argc, const char* argv[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::CONNECTION_ID_TYPE connId;

  if (argc < 2) {
    std::cerr << "Publisher: requires number parameter" << argc << std::endl;
    status = FACE::INVALID_PARAM;
  } else {
    FACE::TS::Initialize("face_config.ini", status);
    if (status != FACE::NO_ERROR) return static_cast<int>(status);

    int part = atoi(argv[1]);
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::MESSAGE_SIZE_TYPE size;
    char connection_name[16];
#if defined OPENDDS_SAFETY_PROFILE
    snprintf(connection_name, sizeof(connection_name), "pub_%d", part);
#else
    ACE_OS::snprintf(connection_name, sizeof(connection_name), "pub_%d", part);
#endif
    FACE::TS::Create_Connection(
      connection_name, FACE::PUB_SUB, connId, dir, size, status);
    if (status != FACE::NO_ERROR) return static_cast<int>(status);

    ACE_OS::sleep(5); // connection established with Subscriber

    std::cout << "Publisher: about to send_message()" << std::endl;
    for (CORBA::Long i = 0; i < part; ++i) {
      if (i) ACE_OS::sleep(1);
      
      Messenger::Message msg = {"Hello, world.", part};
      FACE::TRANSACTION_ID_TYPE txn;
      std::cout << "  sending part " << part << std::endl;
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
      if (status != FACE::NO_ERROR) break;
    }
  }

  ACE_OS::sleep(10); // Subscriber receives message

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return static_cast<int>(status);
}
