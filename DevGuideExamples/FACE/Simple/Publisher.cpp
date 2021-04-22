#include "FaceMessage_TS.hpp"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_unistd.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

// FUZZ: disable check_for_improper_main_declaration
int main(int, char*[])
{
  // Initialize the TS interface
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Create the pub connection
  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE max_msg_size;
  FACE::TS::Create_Connection(
    "pub", FACE::PUB_SUB, connId, dir,
    max_msg_size, FACE::INF_TIME_VALUE, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Message to send
  Messenger::Message msg;
  

    std::string text = "";
    std::string a10 = "aaaaaaaaaa";// 10 "a"s
    std::string a100 = a10 + a10 + a10 + a10 + a10 + a10 + a10 + a10 + a10 + a10; // 100 "a"s
    std::string a1000 = a100 + a100 + a100 + a100 + a100 + a100 + a100 + a100 + a100 + a100; // 1000 "a"s
    
    // If changed to i<100, the message successfully sends
    for(int i=0; i<200; i++) {
      text = text + a1000; 
    }
    printf("Saving message\n");
    

  //printf("size of string = %d\n", sizeof(text));
  //msg.text = "Hello, World!";
  const char* textMsg = text.c_str();
  msg.text = textMsg;
  msg.subject_id = 14;
  msg.count = 1;

  // Send message
  FACE::TRANSACTION_ID_TYPE txn;
  ACE_DEBUG((LM_INFO, "Publisher: about to Send_Message()\n"));
  FACE::TS::Send_Message(
    connId, FACE::INF_TIME_VALUE, txn, msg,
    max_msg_size, status);
  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  // Give message time to be processed before exiting
  ACE_OS::sleep(15);

  // Destroy the pub connection
  FACE::TS::Destroy_Connection(connId, status);

  if (status != FACE::RC_NO_ERROR) {
    return static_cast<int>(status);
  }

  return EXIT_SUCCESS;
}
// FUZZ: enable check_for_improper_main_declaration
