#include "Idl/FaceMessage_TS.hpp"

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
  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connId, dir, size,
                              FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId_INF;
  FACE::CONNECTION_DIRECTION_TYPE dir_INF;
  FACE::MESSAGE_SIZE_TYPE size_INF;
  FACE::TS::Create_Connection("pub_INF", FACE::PUB_SUB, connId_INF, dir_INF,
                              size_INF, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  ACE_OS::sleep(10); // connection established with Subscriber

  // Test timeout settings for send

  std::cout << "Publisher: about to test timeout values in send_message()" << std::endl;
  FACE::Long send_counter = 0;
  FACE::TRANSACTION_ID_TYPE txn_INF;
  FACE::RETURN_CODE_TYPE timeout_tests_status;
  {
    std::cout << "Test 1: sending with TIMEOUT=1 MAX_BLOCKING=INF, should return INVALID_PARAM" << std::endl;
    Messenger::Message msg_INF = {"Hello, world.", send_counter, 0};
    FACE::TS::Send_Message(connId_INF, 1, txn_INF, msg_INF, size_INF, timeout_tests_status);
    if (timeout_tests_status != FACE::INVALID_PARAM) {
      std::cout << "Test 1: ERROR: Send with TIMEOUT=1 MAX_BLOCKING=INF did not fail." << std::endl;
      return static_cast<int>(timeout_tests_status);
    } else {
      std::cout << "Test 1: PASSED" << std::endl;
      timeout_tests_status = FACE::RC_NO_ERROR;
    }
    ACE_OS::sleep(1);
  }
  {
    std::cout << "Test 2: sending with TIMEOUT=0 MAX_BLOCKING=Default (100000000 nsec), should return INVALID_PARAM" << std::endl;
    Messenger::Message msg_INF = {"Hello, world.", send_counter, 0};
    FACE::TS::Send_Message(connId, 0, txn_INF, msg_INF, size, timeout_tests_status);
    if (timeout_tests_status != FACE::INVALID_PARAM) {
      std::cout << "Test 2: ERROR: Send with TIMEOUT=0 MAX_BLOCKING=Default (100000000 nsec) did not fail." << std::endl;
      return static_cast<int>(timeout_tests_status);
    } else {
      std::cout << "Test 2: PASSED" << std::endl;
      timeout_tests_status = FACE::RC_NO_ERROR;
    }
    ACE_OS::sleep(1);
  }

  {
    std::cout << "Test 3: sending msg " << send_counter << " with TIMEOUT=100000000 nsec MAX_BLOCKING=Default (100000000 nsec), should succeed" << std::endl;
    Messenger::Message msg_INF = {"Hello, world.", send_counter, 0};
    int retries = 40;
    do {
      if (timeout_tests_status == FACE::TIMED_OUT) {
        std::cout << "Send_Message timed out, resending " << send_counter << std::endl;
        --retries;
      }
      FACE::TS::Send_Message(connId, 100000000, txn_INF, msg_INF, size_INF, timeout_tests_status);
      ACE_OS::sleep(1);
    } while (timeout_tests_status == FACE::TIMED_OUT && retries > 0);
    if (timeout_tests_status != FACE::RC_NO_ERROR) {
      std::cout << "Test 3: ERROR: Send with TIMEOUT=100000000 nsec MAX_BLOCKING=Default (100000000 nsec) did not succeed." << std::endl;
      return static_cast<int>(timeout_tests_status);
    } else {
      std::cout << "Test 3: PASSED" << std::endl;
      ++send_counter;
    }
  }
  {
    std::cout << "Test 4: sending msg " << send_counter << " with TIMEOUT=200000000 nsec MAX_BLOCKING=Default (100000000 nsec), should succeed" << std::endl;
    Messenger::Message msg_INF = {"Hello, world.", send_counter, 0};
    int retries = 40;
    do {
      if (timeout_tests_status == FACE::TIMED_OUT) {
        std::cout << "Send_Message timed out, resending " << send_counter << std::endl;
        --retries;
      }
      FACE::TS::Send_Message(connId, 200000000, txn_INF, msg_INF, size_INF, timeout_tests_status);
      ACE_OS::sleep(1);
    } while (timeout_tests_status == FACE::TIMED_OUT && retries > 0);
    if (timeout_tests_status != FACE::RC_NO_ERROR) {
      std::cout << "Test 4: ERROR: Send with TIMEOUT=200000000 nsec MAX_BLOCKING=Default (100000000 nsec) did not succeed." << std::endl;
      return static_cast<int>(timeout_tests_status);
    } else {
      std::cout << "Test 4: PASSED" << std::endl;
      ++send_counter;
    }
  }
  // End Testing of timeout settings for send

  std::cout << "Publisher: about to send_message()" << std::endl;
  for (FACE::Long i = send_counter; i < 20; ++i) {
    Messenger::Message msg = {"Hello, world.", i, 0};
    FACE::TRANSACTION_ID_TYPE txn;
    std::cout << "  sending " << i << std::endl;
    int retries = 40;
    do {
      if (status == FACE::TIMED_OUT) {
        std::cout << "Send_Message timed out, resending " << i << std::endl;
        --retries;
      }
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
      ACE_OS::sleep(1);
    } while (status == FACE::TIMED_OUT && retries > 0);

    if (status != FACE::RC_NO_ERROR) break;
  }

  ACE_OS::sleep(15); // Subscriber receives messages

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status_INF = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId_INF, destroy_status_INF);
  if ((destroy_status_INF != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status_INF;
  }
  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  if (status) {
    std::cout << "Publisher status " << status << std::endl;
  }
  return static_cast<int>(status);
}
