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
  DDS::ReturnCode_t waitForSubscriber();
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
  if (waitForSubscriber() != DDS::RETCODE_OK) return 1;

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

DDS::ReturnCode_t Publisher::waitForSubscriber()
{
  std::cout << "Publisher waiting for subscriber..." << std::endl;
  DDS::StatusCondition_var statusCondition = writer->get_statuscondition();
  DDS::ReturnCode_t ret = statusCondition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: set_enabled_statuses()\n")));
    return ret;
  }
  DDS::WaitSet_var waitSet = new DDS::WaitSet;
  ret = waitSet->attach_condition(statusCondition);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: attach_condition()\n")));
    return ret;
  }
  try {
    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::PublicationMatchedStatus match = {0, 0, 0, 0, 0};
    while (match.current_count < Domain::N_READER) {
      ret = waitSet->wait(conditions, timeout);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: wait()\n")));
        throw 1;
      }
      assert(writer->get_status_changes() & DDS::PUBLICATION_MATCHED_STATUS);
      ret = writer->get_publication_matched_status(match);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: get_publication_matched_status()\n")));
        throw 1;
      }
    }
    std::cout << "Publisher match.current_count " << match.current_count << std::endl;
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ERROR: Publisher::waitForSubscriber()\n")));
  }
  DDS::ReturnCode_t ret2 = waitSet->detach_condition(statusCondition);
  return (ret != DDS::RETCODE_OK) ? ret : ret2;
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
