#include "Domain.h"
#include "DataReaderListenerImpl.h"
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
    Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, bool reliable);
    int wait();
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
}

int Subscriber::run() {
  Reader reader1("Reader1", sub_, domain_, reliable_[0]);
  Reader reader2("Reader2", sub_, domain_, reliable_[1]);
  int r1 = reader1.wait();
  int r2 = reader2.wait();
  return r1 + r2;
}

Subscriber::Reader::Reader(const std::string& name, DDS::Subscriber_var& s, const Domain& d, bool reliable)
  : listener_(new DataReaderListenerImpl(name))
{
  try {
    std::cout << name << ": " << (reliable ? "reliable" : "best-effort") << std::endl;
    DDS::DataReaderQos qos;
    s->get_default_datareader_qos(qos);
    qos.reliability.kind = reliable ? DDS::RELIABLE_RELIABILITY_QOS : DDS::BEST_EFFORT_RELIABILITY_QOS;
    reader_ = s->create_datareader(d.topic.in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(reader_.in())) {
      throw ACE_TEXT("create_datareader failed.");
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception in Subscriber::Reader:");
    throw;
  }
}

int Subscriber::Reader::wait()
{
  int ret = 0;
  // Block until Publisher completes
  DDS::StatusCondition_var statusCondition = reader_->get_statuscondition();
  statusCondition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
  DDS::WaitSet_var waitSet = new DDS::WaitSet;
  waitSet->attach_condition(statusCondition);
  try {
    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::SubscriptionMatchedStatus match = {0, 0, 0, 0, 0};
    while (!(match.current_count == 0 && match.total_count > 0)) {
      if (waitSet->wait(conditions, timeout) != DDS::RETCODE_OK) {
        throw ACE_TEXT("ERROR: wait() failed!\n");
      }
      if (reader_->get_subscription_matched_status(match) != DDS::RETCODE_OK) {
        throw ACE_TEXT("ERROR: get_subscription_matched_status failed!\n");
      }
    }
    DataReaderListenerImpl* listener = dynamic_cast<DataReaderListenerImpl*>(listener_.in());
    if(!listener || !listener->valid()) { ret = 1; }
  } catch (...) { ret = 1; }
  waitSet->detach_condition(statusCondition);
  return ret;
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
