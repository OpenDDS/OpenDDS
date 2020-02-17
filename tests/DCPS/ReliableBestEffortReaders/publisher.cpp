#include "Domain.h"
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <assert.h>
#include <iostream>

class Publisher
{
public:
  Publisher(int argc, ACE_TCHAR* argv[]);
  int run();
private:
  void waitForSubscriber();
  Domain domain;
  Messenger::MessageDataWriter_var writer;
};

Publisher::Publisher(int argc, ACE_TCHAR* argv[]) : domain(argc, argv, "Publisher")
{
  DDS::Publisher_var pub = domain.participant->create_publisher(PUBLISHER_QOS_DEFAULT,
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

  DDS::DataWriter_var dw = pub->create_datawriter(domain.topic.in(), qos,
    DDS::DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(dw.in())) {
    throw ACE_TEXT("create_datawriter failed.");
  }
  writer = Messenger::MessageDataWriter::_narrow(dw);
  if (CORBA::is_nil(writer.in())) {
    throw ACE_TEXT("TestMsgDataWriter::_narrow failed.");
  }
}

int Publisher::run()
{
  waitForSubscriber();

  Messenger::Message msg = {"", "", 1, "test", 0, 0, 0};
  DDS::InstanceHandle_t handle = writer->register_instance(msg);
  for (CORBA::Long i = 1; i <= Domain::N_MSG; ++i) {
    msg.count = i;
    DDS::ReturnCode_t r = writer->write(msg, handle);
    if (r != ::DDS::RETCODE_OK) {
      std::cerr << "Publisher write returned error code " << r << std::endl;
    }
  }
  return 0;
}

void Publisher::waitForSubscriber()
{
  std::cout << "Publisher waiting for subscriber..." << std::endl;
  DDS::StatusCondition_var statusCondition = writer->get_statuscondition();
  statusCondition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var waitSet = new DDS::WaitSet;
  waitSet->attach_condition(statusCondition);
  try {
    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::PublicationMatchedStatus match = {0, 0, 0, 0, 0};
    while (match.current_count < Domain::N_READER) {
      waitSet->wait(conditions, timeout);
      assert(writer->get_status_changes() & DDS::PUBLICATION_MATCHED_STATUS);
      writer->get_publication_matched_status(match);
    }
    std::cout << "Publisher match.current_count " << match.current_count << std::endl;
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: Publisher::waitForSubscriber()\n")));
  }
  waitSet->detach_condition(statusCondition);
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
