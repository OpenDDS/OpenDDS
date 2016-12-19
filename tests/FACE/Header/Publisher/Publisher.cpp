#include "../Idl/FaceHeaderTestMsg_TS.hpp"
#include "../Idl/FaceHeaderTestMsgTypeSupportImpl.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/FACE/FaceTSS.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_time.h"
#include <iostream>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  FACE::RETURN_CODE_TYPE status;
  FACE::TS::Initialize("face_config.ini", status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);

  FACE::CONNECTION_ID_TYPE connId;
  FACE::CONNECTION_DIRECTION_TYPE dir;
  FACE::MESSAGE_SIZE_TYPE size;
  FACE::TS::Create_Connection("pub", FACE::PUB_SUB, connId, dir, size, FACE::INF_TIME_VALUE, status);
  if (status != FACE::RC_NO_ERROR) return static_cast<int>(status);


  ACE_OS::sleep(10); // connection established with Subscriber

  std::cout << "Publisher: about to send_message() 10x for callbacks" << std::endl;
  FACE::Long i = 0;

  //Use to get this datawriters repo id for creation of the message instance guid
  OpenDDS::FaceTSS::Entities::ConnIdToSenderMap& writers = OpenDDS::FaceTSS::Entities::instance()->senders_;
  typedef OpenDDS::DCPS::DDSTraits<HeaderTest::Message>::DataWriterType DataWriter;
  const DataWriter::_var_type typedWriter =
    DataWriter::_narrow(writers[connId].dw);
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(writers[connId].dw->get_publisher()->get_participant());
  const OpenDDS::DCPS::RepoId pub = dpi->get_repoid(typedWriter->get_instance_handle());

  for (; i < 10; ++i) {
    //Increment i by 2 for seq number (1 for initial ctl msg and 1 due to txn id start at 1 not 0 like seq numbers)
    HeaderTest::Message msg = {"Hello, world.", i, OpenDDS::FaceTSS::create_message_instance_guid(pub, i+2)};
    FACE::TRANSACTION_ID_TYPE txn;
    std::cout << "  sending " << i << std::endl;
    int retries = 40;
    do {
      if (status == FACE::TIMED_OUT) {
        if (retries % 5 == 0) {
          std::cout << "Send_Message timed out (x5), keep trying, resending msg " << i << std::endl;
        }
        --retries;
      }
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
    } while (status == FACE::TIMED_OUT && retries > 0);

    if (status != FACE::RC_NO_ERROR) break;
  }
  std::cout << "Sleep - wait for callback to unregister" << std::endl;
  ACE_OS::sleep(20); // Subscriber receives messages
  std::cout << "Sleep - done waiting for callback to unregister" << std::endl;

  std::cout << "Publisher: about to send_message() 10x for receives" << std::endl;
  for (; i < 20; ++i) {
    //Increment i by 2 for seq number (1 for initial ctl msg and 1 due to txn id start at 1 not 0 like seq numbers)
    HeaderTest::Message msg = {"Hello, world.", i, OpenDDS::FaceTSS::create_message_instance_guid(pub, i+2)};
    FACE::TRANSACTION_ID_TYPE txn;
    std::cout << "  sending " << i << std::endl;
    int retries = 40;
    do {
      if (status == FACE::TIMED_OUT) {
        if (retries % 5 == 0) {
          std::cout << "Send_Message timed out (x5), keep trying, resending msg " << i << std::endl;
        }
        --retries;
      }
      FACE::TS::Send_Message(connId, FACE::INF_TIME_VALUE, txn, msg, size, status);
    } while (status == FACE::TIMED_OUT && retries > 0);

    if (status != FACE::RC_NO_ERROR) break;
  }

  ACE_OS::sleep(15); // Subscriber receives messages

  // Always destroy connection, but don't overwrite bad status
  FACE::RETURN_CODE_TYPE destroy_status = FACE::RC_NO_ERROR;
  FACE::TS::Destroy_Connection(connId, destroy_status);
  if ((destroy_status != FACE::RC_NO_ERROR) && (!status)) {
    status = destroy_status;
  }

  return static_cast<int>(status);
}
