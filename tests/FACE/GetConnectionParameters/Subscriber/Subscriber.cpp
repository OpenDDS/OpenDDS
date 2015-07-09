#include "Idl/FaceMessage_TS.hpp"

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
  if (callback_count % 10 == 0) {
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
  FACE::MESSAGE_SIZE_TYPE max_msg_size;

  if (!status) {
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, max_msg_size, FACE::INF_TIME_VALUE, status);
  }

  if (!status) {
    FACE::CONNECTION_NAME_TYPE name = {};
    FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
    FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
    if (status != FACE::NOT_AVAILABLE) {
      std::cout << "ERROR: Get_Connection_Parameters after creating connection returned " << status
                << " not FACE::NOT_AVAILABLE (" << FACE::NOT_AVAILABLE << ")" << std::endl;
      return EXIT_FAILURE;
    }
    status = FACE::RC_NO_ERROR;
  }

  ACE_DEBUG((LM_INFO, "Subscriber: about to Register_Callback()\n"));
  FACE::TS::Register_Callback(connId, 0, callback, max_msg_size, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  bool post_reg_callback_check_ok = true;
  if (!status) {
    FACE::CONNECTION_NAME_TYPE name = {};
    FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
    FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
    if (status != FACE::RC_NO_ERROR) {
      std::cout << "ERROR: Get_Connection_Parameters after registering callback returned " << status << std::endl;
      return static_cast<int>(status);
    }
    if (connectionStatus.CONNECTION_DIRECTION != FACE::DESTINATION ||
        connectionStatus.LAST_MSG_VALIDITY != FACE::INVALID ||
        connectionStatus.MESSAGE != 0 ||
        connectionStatus.MAX_MESSAGE != 300 ||
        connectionStatus.MAX_MESSAGE_SIZE != 300 ||
        connectionStatus.WAITING_PROCESSES_OR_MESSAGES != 0 ||
        connectionStatus.REFRESH_PERIOD != 0) {
      std::cout << "ERROR: Get_Connection_Parameters after registering callback  Connection Status values were not as expected" << std::endl;
      post_reg_callback_check_ok = false;
    } else {
      std::cout << "Get_Connection_Parameters after registering callback Connection Status values were as expected" << std::endl;
    }
    std::cout << "\tMESSAGE: " << connectionStatus.MESSAGE << " should be 0\n"
              << "\tMAX_MESSAGE: " << connectionStatus.MAX_MESSAGE << " should be " << 300 << "\n"
              << "\tMAX_MESSAGE_SIZE: " << connectionStatus.MAX_MESSAGE_SIZE << " should be 300\n"
              << "\tCONNECTION_DIRECTION: " << connectionStatus.CONNECTION_DIRECTION << " should be " << FACE::DESTINATION << "\n"
              << "\tWAITING_PROCESSES_OR_MESSAGES: " << connectionStatus.WAITING_PROCESSES_OR_MESSAGES << " should be 0\n"
              << "\tREFRESH_PERIOD: " << connectionStatus.REFRESH_PERIOD << " should be 0\n"
              << "\tLAST_MSG_VALIDITY: " << connectionStatus.LAST_MSG_VALIDITY << " should be " << FACE::INVALID << std::endl;
  }
  if (!post_reg_callback_check_ok ) {
    return EXIT_FAILURE;
  }

  bool receiveMessageHappened = false;
  int recv_msg_count = 0;
  bool firstReceive = true;
  if (!status) {
    const FACE::TIMEOUT_TYPE timeout = FACE::INF_TIME_VALUE;
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
      if (firstReceive) {
        //Let messages queue up so that we can check in connection parameters
        // that WAITING_PROCESSES_OR_MESSAGES > 0
        std::cout << "Sleep for 3 seconds to allow WAITING_PROCESSES_OR_MESSAGES to become > 1 after first receive" << std::endl;
        ACE_OS::sleep(3);
      }
      if ((msg.count != expected) && expected > 0) {
        std::cerr << "ERROR: Expected count " << expected << ", got "
                  << msg.count << std::endl;
        status = FACE::INVALID_PARAM;
        break;
      } else {
        FACE::CONNECTION_NAME_TYPE name = {};
        FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
        FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
        if (status == FACE::RC_NO_ERROR) {
          std::cout << "Get_Connection_Parameters after receiving msg: " << msg.count << "\n"
                    << "\tMESSAGE: " << connectionStatus.MESSAGE << "\n"
                    << "\tMAX_MESSAGE: " << connectionStatus.MAX_MESSAGE << "\n"
                    << "\tMAX_MESSAGE_SIZE: " << connectionStatus.MAX_MESSAGE_SIZE << "\n"
                    << "\tCONNECTION_DIRECTION: " << connectionStatus.CONNECTION_DIRECTION << "\n"
                    << "\tWAITING_PROCESSES_OR_MESSAGES: " << connectionStatus.WAITING_PROCESSES_OR_MESSAGES << "\n"
                    << "\tREFRESH_PERIOD: " << connectionStatus.REFRESH_PERIOD << "\n"
                    << "\tLAST_MSG_VALIDITY: " << connectionStatus.LAST_MSG_VALIDITY << std::endl;
          if (firstReceive) {
            if (connectionStatus.WAITING_PROCESSES_OR_MESSAGES < 1) {
              std::cout << "ERROR: WAITING_PROCESSES_OR_MESSAGES was < 1 after first receive"
                        << " (sleep after first receive should force messages to queue up)" << std::endl;
            } else {
              std::cout << "SUCCESS: WAITING_PROCESSES_OR_MESSAGES was > 0 (actual value: "
                        << connectionStatus.WAITING_PROCESSES_OR_MESSAGES << ") after first receive" << std::endl;
            }
            firstReceive = false;
          }
        } else {
          std::cout << "ERROR: Status after Get_Connection_Parameters was: " << status << std::endl;
          break;
        }

        expected = msg.count + 1;
      }
    }
  }
  bool testPassed = true;

  if (!status) {
    FACE::CONNECTION_NAME_TYPE name = {};
    FACE::TRANSPORT_CONNECTION_STATUS_TYPE connectionStatus;
    FACE::TS::Get_Connection_Parameters(name, connId, connectionStatus, status);
    if (status != FACE::RC_NO_ERROR) {
      std::cout << "ERROR: Get_Connection_Parameters after all messages received returned " << status << std::endl;
      return static_cast<int>(status);
    }
    if (connectionStatus.CONNECTION_DIRECTION != FACE::DESTINATION ||
        connectionStatus.LAST_MSG_VALIDITY != FACE::VALID ||
        connectionStatus.MESSAGE != 0 ||
        connectionStatus.MAX_MESSAGE != 300 ||
        connectionStatus.MAX_MESSAGE_SIZE != 300 ||
        connectionStatus.WAITING_PROCESSES_OR_MESSAGES != 0 ||
        connectionStatus.REFRESH_PERIOD <= 0) {
      std::cout << "ERROR: Get_Connection_Parameters after all messages received Connection Status values were not as expected" << std::endl;
      testPassed = false;
    } else {
      std::cout << "Get_Connection_Parameters after all messages received Connection Status values were as expected" << std::endl;
    }
    std::cout << "\tMESSAGE: " << connectionStatus.MESSAGE << " should be 0\n"
              << "\tMAX_MESSAGE: " << connectionStatus.MAX_MESSAGE << " should be " << 300 << "\n"
              << "\tMAX_MESSAGE_SIZE: " << connectionStatus.MAX_MESSAGE_SIZE << " should be 300\n"
              << "\tCONNECTION_DIRECTION: " << connectionStatus.CONNECTION_DIRECTION << " should be " << FACE::DESTINATION << "\n"
              << "\tWAITING_PROCESSES_OR_MESSAGES: " << connectionStatus.WAITING_PROCESSES_OR_MESSAGES << " should be 0\n"
              << "\tREFRESH_PERIOD: " << connectionStatus.REFRESH_PERIOD << " should be > 0\n"
              << "\tLAST_MSG_VALIDITY: " << connectionStatus.LAST_MSG_VALIDITY << " should be " << FACE::VALID << std::endl;
  }
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
