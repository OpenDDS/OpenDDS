#include "../Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include <iostream>
#include <cstring>

bool callbackHappened = false;
int callback_count = 0;

void callback(FACE::TRANSACTION_ID_TYPE,
              Messenger::Message& msg,
              FACE::MESSAGE_TYPE_GUID,
              FACE::MESSAGE_SIZE_TYPE,
              const FACE::WAITSET_TYPE,
              FACE::RETURN_CODE_TYPE& return_code)
{
  ++callback_count;
  std::cout << "In callback() (the " << callback_count << " time) : "
            << msg.text << '\t' << msg.count << std::endl;
  callbackHappened = true;
  return_code = FACE::RC_NO_ERROR;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  const bool useCallback = argc > 1 && 0 == std::strcmp(argv[1], "callback");

  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  bool testPassed = true;
  if (useCallback) {
    std::cout << "Subscriber: about to Register_Callback()" << std::endl;
    FACE::TS::Register_Callback(connId, 0, callback, 0, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    FACE::TS::Register_Callback(connId, 0, callback, 0, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    ACE_OS::sleep(15);
    if (!callbackHappened || callback_count != 2) {
      std::cout << "ERROR: number callbacks seen incorrect (seen: " << callback_count << " expected: 2)" << std::endl;
      testPassed = false;
    }
    FACE::TS::Unregister_Callback(connId, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  } else {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    std::cout << "Subscriber: about to Receive_Message()" << std::endl;
    FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    std::cout << msg.text << '\t' << msg.count << std::endl;
#ifdef ACE_HAS_CDR_FIXED
    std::cout << msg.deci;
    if (msg.deci != FACE::Fixed("987.654")) {
      std::cout << "ERROR: invalid fixed data " << msg.deci << std::endl;
    }
#endif
  }

  FACE::CONNECTION_NAME_TYPE name = {};
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
  FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.LAST_MSG_VALIDITY != FACE::VALID) {
    std::cout << "ERROR: unexpected value in connection parameters after receiving\n";
    return EXIT_FAILURE;
  }

  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return testPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
