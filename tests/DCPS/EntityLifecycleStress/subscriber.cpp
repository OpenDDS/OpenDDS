#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/unique_ptr.h>
#ifndef ACE_HAS_CPP11
#  include <dds/DCPS/ConditionVariable.h>
#endif
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <tests/Utils/ExceptionStreams.h>
#include <tests/Utils/StatusMatching.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>

#include <memory>
#include <sstream>
#ifdef ACE_HAS_CPP11
#  include <condition_variable>
#  include <mutex>
#  include <thread>
#endif

using namespace Messenger;
using namespace std;

struct DataReaderListenerImpl : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
  DataReaderListenerImpl()
   : mutex_()
#ifdef ACE_HAS_CPP11
   , cv_()
#else
   , cv_(mutex_)
#endif
   , valid_data_seen_(false) {}

  virtual ~DataReaderListenerImpl() {}

  virtual void on_requested_deadline_missed(
    DDS::DataReader*,
    const DDS::RequestedDeadlineMissedStatus&) {}
  virtual void on_requested_incompatible_qos(
    DDS::DataReader*,
    const DDS::RequestedIncompatibleQosStatus&) {}
  virtual void on_liveliness_changed(
    DDS::DataReader*,
    const DDS::LivelinessChangedStatus&) {}
  virtual void on_subscription_matched(
    DDS::DataReader*,
    const DDS::SubscriptionMatchedStatus&) {}
  virtual void on_sample_rejected(
    DDS::DataReader*,
    const DDS::SampleRejectedStatus&) {}
  virtual void on_sample_lost(
    DDS::DataReader*,
    const DDS::SampleLostStatus&) {}

  virtual void on_data_available(DDS::DataReader* reader)
  {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);
    if (0 == message_dr) {
      return;
    }

    Messenger::Message message;
    DDS::SampleInfo si;

    if (message_dr->take_next_sample(message, si) == DDS::RETCODE_OK) {
      if (si.valid_data) {
        stringstream ss;
        ss << "Subscriber " << ACE_OS::getpid() << " got new message data:" << endl;
        ss << " - From  : " << message.from << endl;
        ss << " - Count : " << message.count << endl;
        cout << ss.str() << flush;
#ifdef ACE_HAS_CPP11
        std::unique_lock<std::mutex> lock(mutex_);
#else
        ACE_Guard<ACE_Thread_Mutex> g(mutex_);
#endif
        valid_data_seen_ = true;
        cv_.notify_all();
      }
    }
  }

  void wait_valid_data(const OpenDDS::DCPS::MonotonicTimePoint& deadline) {
#ifdef ACE_HAS_CPP11
    std::chrono::milliseconds ms((deadline - OpenDDS::DCPS::MonotonicTimePoint::now()).value().get_msec());
    std::unique_lock<std::mutex> lock(mutex_);
#else
    ACE_Guard<ACE_Thread_Mutex> g(mutex_);
#endif
#ifdef ACE_HAS_CPP11
    while (!valid_data_seen_ && cv_.wait_for(lock, ms) != cv_status::timeout) {
#else
    OpenDDS::DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
    while (!valid_data_seen_ && cv_.wait_until(deadline, thread_status_manager) != OpenDDS::DCPS::CvStatus_Timeout) {
#endif
    }
  }

private:
#ifdef ACE_HAS_CPP11
  std::mutex mutex_;
  std::condition_variable cv_;
#else
  ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cv_;
#endif
  bool valid_data_seen_;
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(31,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    MessageTypeSupportImpl::_var_type servant = new MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != servant->register_type(participant.in(), "")) {
      cerr << "register_type failed." << endl;
      exit(1);
    }

    CORBA::String_var type_name = servant->get_type_name();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                topic_qos,
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    DDS::SubscriberQos sub_qos;
    participant->get_default_subscriber_qos(sub_qos);

    DDS::Subscriber_var sub =
      participant->create_subscriber(sub_qos, DDS::SubscriberListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub.in())) {
      cerr << "create_subscriber failed." << endl;
      exit(1);
    }

    DataReaderListenerImpl* drli = new DataReaderListenerImpl();
    DDS::DataReaderListener_var drl = drli;

    DDS::DataReader_var dr =
      sub->create_datareader(topic.in(),
                              DATAREADER_QOS_DEFAULT,
                              drl.in(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dr.in())) {
      cerr << "create_datareader failed." << endl;
      exit(1);
    }

    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(dr.in());

    if (CORBA::is_nil(message_dr.in())) {
      cerr << "Messenger::MessageDataReader::_narrow() failed." << endl;
      exit(1);
    }

    const OpenDDS::DCPS::MonotonicTimePoint deadline = OpenDDS::DCPS::MonotonicTimePoint::now() + OpenDDS::DCPS::TimeDuration(7, 500000);

    drli->wait_valid_data(deadline);

    {
      stringstream ss;
      ss << "Subscriber " << ACE_OS::getpid() << " is done. Exiting." << endl;

      cout << ss.str() << flush;
    }

    // Semi-arbitrary change to clean-up approach
    if (ACE_OS::getpid() % 3) {
      message_dr = 0;

      sub->delete_datareader(dr);
      participant->delete_subscriber(sub);
    }

#ifdef ACE_HAS_CPP11
    std::atomic<bool> still_cleaning(true);
    thread t([&](){
      this_thread::sleep_for(chrono::seconds(3));
      while (still_cleaning) {
        stringstream ss;
        ss << "Subscriber " << ACE_OS::getpid() << " is taking a long time to clean up." << endl;
        cout << ss.str() << flush;
        this_thread::sleep_for(chrono::seconds(1));
      }
    });
#endif

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

#ifdef ACE_HAS_CPP11
    still_cleaning = false;
    t.join();
#endif
  }
  catch (const CORBA::Exception& e)
  {
     cerr << "PUB: Exception caught in main.cpp:" << endl
       << e << endl;
     exit(1);
  }
  TheServiceParticipant->shutdown();

  return 0;
}
