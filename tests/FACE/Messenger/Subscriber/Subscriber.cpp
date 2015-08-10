#include "../Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/Log_Msg.h"
#include <cstring>

#include "tests/Utils/Safety.h"

bool callbackHappened = false;
int callback_count = 0;

void callback(FACE::TRANSACTION_ID_TYPE,
              Messenger::Message& msg,
              FACE::MESSAGE_TYPE_GUID message_type_id,
              FACE::MESSAGE_SIZE_TYPE message_size,
              const FACE::WAITSET_TYPE,
              FACE::RETURN_CODE_TYPE& return_code)
{
  ++callback_count;
  ACE_DEBUG((LM_INFO, "In callback() (the %d time): %C\t%d\t"
             "message_type_id: %Ld\tmessage_size: %d\n",
             callback_count, msg.text.in(), msg.count,
             message_type_id, message_size));
  callbackHappened = true;
  return_code = FACE::RC_NO_ERROR;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  if (argc < 2) {
    ACE_ERROR((LM_ERROR, "No config file\n"));
    return EXIT_FAILURE;
  }
  const char* config = argv[1];
  const bool useCallback = argc > 2 && 0 == std::strcmp(argv[2], "callback");
  const char* subname = useCallback ? "suba" : "subb";

  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize(config, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  DisableGlobalNew dgn;

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;
  FACE::TS::Create_Connection(subname, FACE::PUB_SUB, connId, dir, max_msg_size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  bool testPassed = true;
  if (useCallback) {
    ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
    FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    ACE_OS::sleep(15);
    if (!callbackHappened || callback_count != 2) {
      ACE_ERROR((LM_ERROR, "ERROR: number callbacks seen incorrect (seen: %d "
        "expected: 2)\n", callback_count));
      testPassed = false;
    }
    FACE::TS::Unregister_Callback(connId, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  } else {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    ACE_DEBUG((LM_INFO, "Subscriber: about to Receive_Message()\n"));
    FACE::TS::Receive_Message(connId, timeout, txn, msg, max_msg_size, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    ACE_DEBUG((LM_INFO, "%C\t%d\n", msg.text.in(), msg.count));
#ifdef ACE_HAS_CDR_FIXED
    if (msg.deci != FACE::Fixed("987.654")) {
      const FACE::String_var decimal = msg.deci.to_string();
      ACE_ERROR((LM_ERROR, "ERROR: invalid fixed data %C\n", decimal.in()));
      testPassed = false;
    }
#endif
  }

  FACE::CONNECTION_NAME_TYPE name = {};
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
  FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  if (connectionStatus.LAST_MSG_VALIDITY != FACE::VALID) {
    ACE_ERROR((LM_ERROR,
               "ERROR: unexpected value in connection parameters after receiving\n"));
    return EXIT_FAILURE;
  }

  FACE::TS::Destroy_Connection(connId, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  return testPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
