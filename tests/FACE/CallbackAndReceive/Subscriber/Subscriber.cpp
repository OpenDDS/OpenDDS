#include "Idl/FaceMessage_TS.hpp"
#include "../../Utils.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include <iostream>

bool callbackHappened = false;
int callback_count = 0;
long expected = 0;
FACE::CONNECTION_ID_TYPE connId;
bool callback_unregistered = false;

void callback(FACE::TRANSACTION_ID_TYPE,
              Messenger::Message& msg,
              FACE::MESSAGE_TYPE_GUID message_type_id,
              FACE::MESSAGE_SIZE_TYPE message_size,
              const FACE::WAITSET_TYPE,
              FACE::RETURN_CODE_TYPE& return_code)
{
  ++callback_count;
  expected = msg.count + 1;
  ACE_DEBUG((LM_INFO, "In callback() (the %d time): %C\t%d\t"
             "message_type_id: %Ld\tmessage_size: %d\n",
             callback_count, msg.text.in(), msg.count,
             message_type_id, message_size));
  callbackHappened = true;
  if (callback_count % 2 == 0) {
    ACE_DEBUG((LM_INFO, "Subscriber: about to Unregister_Callback()\n"));
    FACE::TS::Unregister_Callback(connId, return_code);
    if (return_code != FACE::RC_NO_ERROR) return;
    callback_unregistered = true;
  }
  return_code = FACE::RC_NO_ERROR;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status = FACE::RC_NO_ERROR;
  FACE::TS::Initialize("face_config.ini", status);
  FACE::MESSAGE_SIZE_TYPE max_msg_size = 0;

  if (!status) {
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, max_msg_size, FACE::INF_TIME_VALUE, status);
  }

  ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
  FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
  bool receiveMessageHappened = false;
  int recv_msg_count = 0;
  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = TestUtils::seconds_to_timeout(20);
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    std::cout << "Subscriber: about to Receive_Message()" << std::endl;
    while (expected <= 19) {
      FACE::TS::Receive_Message(connId, timeout, txn, msg, max_msg_size, status);
      if (status != FACE::RC_NO_ERROR) break;
      ACE_DEBUG((LM_INFO, "Receive_Message: (the %d time): %C\t%d\t"
                 "ttid: %Ld\n",
                 recv_msg_count, msg.text.in(), msg.count,
                 txn));
      ++recv_msg_count;
      receiveMessageHappened = true;
      if (expected % 2 != 0) {
        ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
        FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
        if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
        expected = msg.count + 1;
      }
    }
    if (status == FACE::TIMED_OUT)
      status = FACE::RC_NO_ERROR; // time out OK when writer is done
  }
  bool testPassed = true;
  if (!callbackHappened) {
    ACE_ERROR((LM_ERROR,
               "ERROR: Callback was not triggered\n"));
    testPassed = false;
  } else {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Callback was triggered %d times\n", callback_count));
  }
  if (!receiveMessageHappened) {
    ACE_ERROR((LM_ERROR,
               "ERROR: Receive Message was not triggered\n"));
    testPassed = false;
  } else {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Receive Message was triggered %d times\n", recv_msg_count));
  }
  if (!callback_unregistered) {
    ACE_DEBUG((LM_INFO, "Subscriber: about to Unregister_Callback()\n"));
    FACE::TS::Unregister_Callback(connId, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
  }

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return testPassed ? static_cast<int>(status) : EXIT_FAILURE;
}
