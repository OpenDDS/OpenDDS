#include <MessengerTypeSupportImpl.h>

#include <dds/DCPS/Atomic.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>
#include <ace/Task.h>

using namespace OpenDDS::DCPS;

namespace {

const DDS::DomainId_t domain = 42;
const size_t writer_count = 16;

class AssertThread : public ACE_Task_Base {
public:
  explicit AssertThread(DDS::DomainParticipant_ptr participant)
    : participant_(DDS::DomainParticipant::_duplicate(participant))
    , stop_(false)
    , calls_(0)
    , errors_(0)
  {}

  int svc()
  {
    while (!stop_.load()) {
      if (participant_->assert_liveliness() != DDS::RETCODE_OK) {
        ++errors_;
      }
      ++calls_;
      ACE_OS::sleep(ACE_Time_Value(0, 100));
    }
    return 0;
  }

  void stop()
  {
    stop_.store(true);
    wait();
  }

  size_t calls() const { return calls_; }
  size_t errors() const { return errors_; }

private:
  DDS::DomainParticipant_var participant_;
  Atomic<bool> stop_;
  size_t calls_;
  size_t errors_;
};

DDS::DataWriter_ptr create_writer(DDS::Publisher_ptr publisher, DDS::Topic_ptr topic,
                                  DDS::LivelinessQosPolicyKind kind)
{
  DDS::DataWriterQos qos;
  publisher->get_default_datawriter_qos(qos);
  qos.liveliness.kind = kind;
  qos.liveliness.lease_duration.sec = 0;
  qos.liveliness.lease_duration.nanosec = 5000000;
  return publisher->create_datawriter(topic, qos, 0, DEFAULT_STATUS_MASK);
}

// PublisherImpl::enable enables the writer while holding the publisher's lock.
bool enable_writer_from_publisher(DDS::DomainParticipant_ptr participant, DDS::Topic_ptr topic)
{
  DDS::Publisher_var publisher =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  if (!publisher) {
    return false;
  }
  bool enabled;
  {
    DDS::DataWriter_var writer = create_writer(publisher, topic, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
    enabled = writer && publisher->enable() == DDS::RETCODE_OK;
  }
  const bool deleted = publisher->delete_contained_entities() == DDS::RETCODE_OK &&
    participant->delete_publisher(publisher) == DDS::RETCODE_OK;
  return enabled && deleted;
}

}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  int seconds = 10;
  ACE_Arg_Shifter args(argc, argv);
  while (args.is_anything_left()) {
    const ACE_TCHAR* arg = args.get_the_parameter(ACE_TEXT("-d"));
    if (arg) {
      seconds = ACE_OS::atoi(arg);
      args.consume_arg();
    } else {
      args.ignore_arg();
    }
  }

  DDS::DomainParticipantQos participant_qos;
  dpf->get_default_participant_qos(participant_qos);
  // Leave publishers disabled when created so PublisherImpl::enable enables their writers.
  participant_qos.entity_factory.autoenable_created_entities = false;
  DDS::DomainParticipant_var participant =
    dpf->create_participant(domain, participant_qos, 0, DEFAULT_STATUS_MASK);
  if (!participant || participant->enable() != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) ERROR: create_participant failed\n"), 1);
  }

  Messenger::MessageTypeSupport_var ts = new Messenger::MessageTypeSupportImpl;
  if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) ERROR: register_type failed\n"), 1);
  }
  CORBA::String_var type_name = ts->get_type_name();
  DDS::Topic_var topic =
    participant->create_topic("LivelinessWriterChurn", type_name, TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  DDS::Publisher_var publisher =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  if (!topic || !publisher || topic->enable() != DDS::RETCODE_OK || publisher->enable() != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) ERROR: create_topic or create_publisher failed\n"), 1);
  }

  AssertThread assert_thread(participant);
  if (assert_thread.activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) ERROR: failed to start assert thread\n"), 1);
  }

  // Replace writers on one publisher and enable writers from new publishers
  // while the participant's liveliness timers and the assert thread walk the publishers' writers.
  int status = 0;
  size_t iterations = 0;
  {
    // A deleted writer keeps its liveliness timer until it is released, so release them before shutdown.
    DDS::DataWriter_var writers[writer_count];
    const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration(seconds);
    while (MonotonicTimePoint::now() < deadline) {
      DDS::DataWriter_var& writer = writers[iterations % writer_count];
      if (writer && publisher->delete_datawriter(writer) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: delete_datawriter failed\n"));
        status = 1;
        break;
      }
      const DDS::LivelinessQosPolicyKind kind = iterations % 2 ?
        DDS::AUTOMATIC_LIVELINESS_QOS : DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      writer = create_writer(publisher, topic, kind);
      if (!writer) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: create_datawriter failed\n"));
        status = 1;
        break;
      }
      if (iterations % 8 == 0 && !enable_writer_from_publisher(participant, topic)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: enable_writer_from_publisher failed\n"));
        status = 1;
        break;
      }
      ++iterations;
    }
  }

  assert_thread.stop();
  if (assert_thread.errors()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: %B assert_liveliness calls failed\n", assert_thread.errors()));
    status = 1;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) replaced %B writers, called assert_liveliness %B times\n",
             iterations, assert_thread.calls()));

  if (participant->delete_contained_entities() != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: delete_contained_entities failed\n"));
    status = 1;
  }
  if (dpf->delete_participant(participant) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: delete_participant failed\n"));
    status = 1;
  }
  TheServiceParticipant->shutdown();
  return status;
}
