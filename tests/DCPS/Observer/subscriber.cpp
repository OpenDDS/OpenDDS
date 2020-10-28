#include "Domain.h"
#include "DataReaderListenerImpl.h"
#include "TestObserver.h"
#include <dds/DCPS/EntityImpl.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <ace/Get_Opt.h>
#include <iostream>

using namespace OpenDDS::DCPS;

class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]);
  int run();

private:
  class Reader {
  public:
    Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, const Observer::Event e);
    int waitForPublisherDone();
    void change_qos();
  private:
    DDS::DataReaderListener_var listener_;
    DDS::DataReader_var reader_;
  };

  Domain domain_;
  DDS::Subscriber_var sub_;
};

Subscriber::Subscriber(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Subscriber")
{
  sub_ = domain_.participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
    DDS::SubscriberListener::_nil(), DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(sub_.in())) {
    throw ACE_TEXT("create_subscriber failed.");
  }

  // register Observer::e_SAMPLE_RECEIVED for all readers in this subscriber
  EntityImpl* entity = dynamic_cast<EntityImpl*>(sub_.ptr());
  Observer_rch o = entity->get_observer(Observer::e_ENABLED);
  entity->set_observer(o, Observer::e_SAMPLE_RECEIVED);
}

int Subscriber::run()
{
  Reader reader1("Reader1", sub_, domain_, Observer::e_SAMPLE_READ);
  Reader reader2("Reader2", sub_, domain_, Observer::e_SAMPLE_TAKEN);
  int r1 = reader1.waitForPublisherDone();
  int r2 = reader2.waitForPublisherDone();
  return r1 + r2;
}

Subscriber::Reader::Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, const Observer::Event e)
  : listener_(new DataReaderListenerImpl(name))
{
  try {
    std::cout << name << " observes " << (e & Observer::e_SAMPLE_READ ? "SAMPLE_READ" : "SAMPLE_TAKEN") << std::endl;
    DDS::DataReaderQos qos;
    if (s->get_default_datareader_qos(qos) != DDS::RETCODE_OK) {
      throw ACE_TEXT("get_default_datareader_qos failed.");
    }
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    reader_ = s->create_datareader(d.topic, qos, listener_, DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(reader_.in())) {
      throw ACE_TEXT("create_datareader failed.");
    }

    // register the specified Observer::Event for this reader
    EntityImpl* entity = dynamic_cast<EntityImpl*>(reader_.ptr());
    Observer_rch o = entity->get_observer(Observer::e_ENABLED);
    entity->set_observer(o, e);

    change_qos();
  } catch (...) { throw; }
}

int Subscriber::Reader::waitForPublisherDone()
{
  // Block until Publisher completes
  DDS::StatusCondition_var status = reader_->get_statuscondition();
  DDS::WaitSet waitSet;
  if (status->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS) != DDS::RETCODE_OK
    || waitSet.attach_condition(status) != DDS::RETCODE_OK) {
    return 1;
  }
  int ret = 0;
  try {
    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::SubscriptionMatchedStatus match = {0, 0, 0, 0, 0};
    while (match.current_count != 0 || match.total_count <= 0) {
      if (waitSet.wait(conditions, timeout) != DDS::RETCODE_OK) {
        throw ACE_TEXT("ERROR: wait() failed!\n");
      }
      if (reader_->get_subscription_matched_status(match) != DDS::RETCODE_OK) {
        throw ACE_TEXT("ERROR: get_subscription_matched_status failed!\n");
      }
    }
  } catch (...) { ret = 1; }
  return (waitSet.detach_condition(status) == DDS::RETCODE_OK ? 0 : 1) + ret;
}

void Subscriber::Reader::change_qos()
{
  DDS::DataReaderQos qos;
  if (reader_->get_qos(qos) == DDS::RETCODE_OK) {
    Domain::change_qos(qos, "cba");
    reader_->set_qos(qos);
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Subscriber sub(argc, argv);
    return sub.run();
  } catch (...) {
    return 1;
  }
}
