#include "FaceMessage_TS.hpp"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_unistd.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

void callback(FACE::TRANSACTION_ID_TYPE,
              Messenger::Message& msg,
              FACE::MESSAGE_TYPE_GUID message_type_id,
              FACE::MESSAGE_SIZE_TYPE message_size,
              const FACE::WAITSET_TYPE,
              FACE::RETURN_CODE_TYPE& return_code)
{
  ACE_DEBUG((LM_INFO,
    "Callback: %C\t%d\n", msg.from.in(), msg.count));
  return_code = FACE::RC_NO_ERROR;
}

int main(int, const char*[])
{
  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Create the sub connection
  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;
  FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, max_msg_size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Register a callback
  ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
  FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Give message time to be processed before exiting
  ACE_OS::sleep(15);

  // Unregister the callback
  FACE::TS::Unregister_Callback(connId, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Destroy the sub connection
  FACE::TS::Destroy_Connection(connId, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  return EXIT_SUCCESS;
}
