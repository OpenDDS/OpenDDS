#include "../Idl/FaceMessage_TSS.hpp"

#include "ace/OS_NS_unistd.h"
#include <iostream>

int main()
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("fake_config.xml", status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::create_connection("pub", FACE::PUB_SUB, connId, dir, size, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);
  ACE_OS::sleep(5); // connection established with Subscriber

  const Messenger::Message msg = {"Hello, world.", 0};
  FACE::TRANSACTION_ID_TYPE txn;
  std::cout << "Publisher: about to send_message()" << std::endl;
  FACE::TS::send_message(connId, msg, size, FACE::INF_TIME_VALUE, txn, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);
  ACE_OS::sleep(5); // Subscriber receives message

  FACE::TS::destroy_connection(connId, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
