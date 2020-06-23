#include "Domain.h"

#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <assert.h>
#include <iostream>

class Publisher
{
public:
  Publisher(int argc, ACE_TCHAR* argv[]);
  int run();
private:
  DDS::ReturnCode_t waitForSubscriber();
  Domain domain_;
  Messenger::MessageDataWriter_var writer_;
};

Publisher::Publisher(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Publisher")
{
  DDS::Publisher_var pub = domain_.participant->create_publisher(PUBLISHER_QOS_DEFAULT,
    DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(pub.in())) {
    throw ACE_TEXT("create_publisher failed.");
  }

  DDS::DataWriterQos qos;
  pub->get_default_datawriter_qos(qos);
  qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
  DDS::Duration_t livelinessLease = {2, 0}; //2 seconds
  qos.liveliness.lease_duration = livelinessLease;

  DDS::DataWriter_var dw = pub->create_datawriter(domain_.topic.in(), qos,
    DDS::DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(dw.in())) {
    throw ACE_TEXT("create_datawriter failed.");
  }
  writer_ = Messenger::MessageDataWriter::_narrow(dw);
  if (CORBA::is_nil(writer_.in())) {
    throw ACE_TEXT("TestMsgDataWriter::_narrow failed.");
  }
}

int Publisher::run()
{
  if (waitForSubscriber() != DDS::RETCODE_OK) return 1;

  Messenger::Message msg = {"", "", 1, "test", 0, 0, 0};
  DDS::InstanceHandle_t handle = writer_->register_instance(msg);
  for (CORBA::Long i = 1; i <= Domain::N_MSG; ++i) {
    if (i > 1) {
      ACE_OS::sleep(ACE_Time_Value(0, 300000)); // sleep 300 ms
    }
    msg.count = i;
    DDS::ReturnCode_t r = writer_->write(msg, handle);
    if (r != ::DDS::RETCODE_OK) {
      std::cerr << "Publisher write returned error code " << r << std::endl;
    }
  }
  return 0;
}

DDS::ReturnCode_t Publisher::waitForSubscriber()
{
  std::cout << "Publisher waiting for subscriber..." << std::endl;
  DDS::DataWriter_var writer = DDS::DataWriter::_narrow(writer_);
  Utils::wait_match(writer, Domain::N_READER);
  return DDS::RETCODE_OK;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Publisher pub(argc, argv);
    return pub.run();
  } catch (...) {
    return 1;
  }
}
