#include "../Idl/FaceMessage_TS.hpp"

#include <iostream>

int main()
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("fake_config.xml", status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);

  const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
  FACE::TRANSACTION_ID_TYPE txn;
  Messenger::Message msg;
  std::cout << "Subscriber: about to receive_message()" << std::endl;
  FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);
  std::cout << msg.text.in() << '\t' << msg.count << std::endl;

  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::NO_ERROR) return static_cast<int>(status);

  return EXIT_SUCCESS;
}
