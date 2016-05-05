#include "../Idl/FaceHeaderTestMsg_TS.hpp"
#include "dds/DCPS/SafetyProfileStreams.h"

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
const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;


void callback(FACE::TRANSACTION_ID_TYPE txn,
              HeaderTest::Message& msg,
              FACE::MESSAGE_TYPE_GUID message_type_id,
              FACE::MESSAGE_SIZE_TYPE message_size,
              const FACE::WAITSET_TYPE,
              FACE::RETURN_CODE_TYPE& return_code)
{
  ++callback_count;
  expected = msg.count + 1;
  std::cout << "In callback() (the " << callback_count << " time): " << msg.text.in()
            << "\t" << msg.count << "\tmsg_instance_guid: " << std::hex << msg.msg_instance_guid
            << std::dec << "\tmessage_type_id: " << message_type_id << "\tmessage_size: "
            << message_size << "\ttransaction_id: " << txn << std::endl;
  callbackHappened = true;
  FACE::TS::MessageHeader hdr;
  FACE::TS::Receive_Message(connId, timeout, txn, hdr, sizeof(FACE::TS::MessageHeader), return_code);
  if (return_code != FACE::RC_NO_ERROR) {
    std::cout << "ERROR: In callback() - Receive header failed for tid: " << txn << " with status: " << return_code << std::endl;
    return;
  }
  std::cout << "In callback() Message Header - tid: " << txn
            << "\n\tplatform view guid: " << hdr.platform_view_guid
            << "\n\tsource timestamp: " << hdr.message_timestamp
            << "\n\tinstance guid: " << std::hex << hdr.message_instance_guid
            << "\n\tsource guid: " << std::dec << hdr.message_source_guid
            << "\n\tvalidity " << hdr.message_validity << std::endl;
  if (hdr.message_source_guid != 9645061) {
    std::cout << "ERROR: Receive_Message for header failed.  Header source guid " << hdr.message_source_guid << " does not equal 9645061" << std::endl;
    return_code = FACE::INVALID_PARAM;
    return;
  }
  if (hdr.message_instance_guid != msg.msg_instance_guid) {
      std::cout << "ERROR: Receive_Message for header failed.  message_instance_guid " << std::hex << hdr.message_instance_guid
                << " != " << msg.msg_instance_guid << std::endl;
      return_code = FACE::INVALID_PARAM;
      return;
  }
  if (callback_count == 10) {
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
    if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);
  }

  //First test receive header for a callback processed message
  //Should receive 10 messages, process/receive their headers then unregister callback
  ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
  FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  // Second test receive header after a Receive_Message processed message
  bool receiveMessageHappened = false;
  int recv_msg_count = 0;
  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
    FACE::TRANSACTION_ID_TYPE txn;
    HeaderTest::Message msg;
    std::cout << "Subscriber: about to Receive_Message()" << std::endl;
    while (expected <= 19) {
      FACE::TS::Receive_Message(connId, timeout, txn, msg, max_msg_size, status);
      if (status != FACE::RC_NO_ERROR) break;
        std::cout << "Receive_Message: (the " << recv_msg_count << " time): " << msg.text.in()
                  << "\t" << msg.count << "\tmsg_instance_guid: " << std::hex << msg.msg_instance_guid
                  << std::dec << "\tttid: " << txn << std::endl;
      ++recv_msg_count;
      receiveMessageHappened = true;
      FACE::TS::MessageHeader hdr;
      FACE::TS::Receive_Message(connId, timeout, txn, hdr, sizeof(FACE::TS::MessageHeader), status);
      if (status != FACE::RC_NO_ERROR) {
        std::cout << "ERROR: Receive_Message for header failed for tid: " << txn << " with status: " << status << std::endl;
        break;
      }
      std::cout << "Message Header - tid: " << txn
                << "\n\tplatform view guid: " << hdr.platform_view_guid
                << "\n\tsource timestamp: " << hdr.message_timestamp
                << "\n\tinstance guid: " << std::hex << hdr.message_instance_guid
                << "\n\tsource guid: " << std::dec << hdr.message_source_guid
                << "\n\tvalidity " << hdr.message_validity << std::endl;
      if (hdr.message_source_guid != 9645061) {
        std::cout << "ERROR: Receive_Message for header failed.  Header source guid " << hdr.message_source_guid
                  << " != 9645061" << std::endl;
        status = FACE::INVALID_PARAM;
        return status;
      }
      if (hdr.message_instance_guid != msg.msg_instance_guid) {
        std::cout << "ERROR: Receive_Message for header failed.  message_instance_guid " << std::hex << hdr.message_instance_guid
                  << " != " << msg.msg_instance_guid << std::endl;
        status = FACE::INVALID_PARAM;
        return status;
      }
      expected = msg.count + 1;
    }
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
