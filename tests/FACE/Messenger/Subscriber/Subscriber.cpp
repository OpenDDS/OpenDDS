#include "../Idl/FaceMessage_TS.hpp"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/Log_Msg.h"
#include <cstring>

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

static bool no_global_new = false;

#if defined(OPENDDS_SAFETY_PROFILE) && (defined(__GLIBC__) || defined(ACE_HAS_EXECINFO_H))
#include <execinfo.h>

void* operator new(size_t sz)
#ifdef ACE_HAS_NEW_THROW_SPEC
  throw (std::bad_alloc)
#endif
 {
  if (no_global_new) {
    printf ("ERROR: call to global operator new\n");
  }
  return ::malloc(sz);
}

void operator delete(void* ptr) {
  ::free(ptr);
}

void* operator new[](size_t sz, const std::nothrow_t&)
#ifdef ACE_HAS_NEW_THROW_SPEC
  throw (std::bad_alloc)
#endif
{
  if (no_global_new) {
    printf ("ERROR: Subscriber call to global operator new[]\n");
    void* addresses[32];
    int count = backtrace(addresses, 32);
    char** text = backtrace_symbols(addresses, count);
    for (int i = 0; i != count; ++i) {
      printf ("Subscriber %s\n", text[i]);
    }
    ::free(text);
  }
  return ::malloc(sz);
}

void operator delete[](void* ptr) {
  ::free(ptr);
}
#endif

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  const bool useCallback = argc > 1 && 0 == std::strcmp(argv[1], "callback");

  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  no_global_new = true;

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  bool testPassed = true;
  if (useCallback) {
    ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
    FACE::TS::Register_Callback(connId, 0, callback, 0, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    FACE::TS::Register_Callback(connId, 0, callback, 0, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
    ACE_OS::sleep(15);
    if (!callbackHappened || callback_count != 2) {
      ACE_ERROR((LM_ERROR,
                 "ERROR: number callbacks seen incorrect (seen: %d expected: 2)\n"));
      testPassed = false;
    }
    FACE::TS::Unregister_Callback(connId, status);
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  } else {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;
    ACE_DEBUG((LM_INFO, "Subscriber: about to Receive_Message()\n"));
    FACE::TS::Receive_Message(connId, timeout, txn, msg, size, status);
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
