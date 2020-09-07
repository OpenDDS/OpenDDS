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

class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]);
  int run();

private:
  class Reader {
  public:
    Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, bool reliable, const OpenDDS::DCPS::Observer::Event e);
    int waitForPublisherDone();
    void change_qos();
  private:
    DDS::DataReaderListener_var listener_;
    DDS::DataReader_var reader_;
  };

  Domain domain_;
  DDS::Subscriber_var sub_;
  bool reliable_[Domain::N_READER];
};

Subscriber::Subscriber(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Subscriber") {
  sub_ = domain_.participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
    DDS::SubscriberListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(sub_.in())) {
    throw ACE_TEXT("create_subscriber failed.");
  }
  reliable_[0] = false;
  reliable_[1] = false;
  ACE_Get_Opt opts(argc, argv, ACE_TEXT("r:"));
  for (int option = opts(); option != EOF; option = opts()) {
    if (option == 'r') {
      ACE_TCHAR* r = opts.opt_arg();
      if (r) {
        reliable_[0] = (r[0] == '1');
        reliable_[1] = (r[1] == '1');
      }
      break;
    }
  }

  // register Observer::e_SAMPLE_RECEIVED for all readers in this subscriber
  auto entity = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(sub_.ptr());
  entity->set_observer(OpenDDS::DCPS::make_rch<TestObserver>(),
                       OpenDDS::DCPS::Observer::e_SAMPLE_RECEIVED);
}

int Subscriber::run() {
  Reader reader1("Reader1", sub_, domain_, reliable_[0], OpenDDS::DCPS::Observer::e_SAMPLE_READ);
  Reader reader2("Reader2", sub_, domain_, reliable_[1], OpenDDS::DCPS::Observer::e_SAMPLE_TAKEN);
  int r1 = reader1.waitForPublisherDone();
  int r2 = reader2.waitForPublisherDone();
  return r1 + r2;
}

Subscriber::Reader::Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, bool reliable, const OpenDDS::DCPS::Observer::Event e)
  : listener_(new DataReaderListenerImpl(name))
{
  try {
    std::cout << name << '(' << (reliable ? "reliable" : "best-effort") << ") observes " <<
      (e & OpenDDS::DCPS::Observer::e_SAMPLE_READ ? "e_SAMPLE_READ" : "e_SAMPLE_TAKEN") << std::endl;
    DDS::DataReaderQos qos;
    s->get_default_datareader_qos(qos);
    qos.reliability.kind = reliable ? DDS::RELIABLE_RELIABILITY_QOS : DDS::BEST_EFFORT_RELIABILITY_QOS;
    reader_ = s->create_datareader(d.topic.in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(reader_.in())) {
      throw ACE_TEXT("create_datareader failed.");
    }

    // register the specified Observer::Event for this reader
    auto entity = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(reader_.ptr());
    entity->set_observer(OpenDDS::DCPS::make_rch<TestObserver>(), e);

    change_qos();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception in Subscriber::Reader:");
    throw;
  }
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
