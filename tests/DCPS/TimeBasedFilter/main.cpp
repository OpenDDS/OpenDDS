/*
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <tao/Basic_Types.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/SubscriptionInstance.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>

#include "FooTypeTypeSupportImpl.h"
#include "FooTypeTypeSupportC.h"

#include "dds/DCPS/StaticIncludes.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Condition_T.h"

namespace
{
static DDS::Duration_t minimum_separation = { 5, 0 };
static bool reliable = false;

static const ::CORBA::Long NUM_INSTANCES = 2;
static const size_t SAMPLES_PER_CYCLE = 5;
static const size_t SEPARATION_MULTIPLIER = 2;

void
parse_args(int& argc, ACE_TCHAR** argv)
{
  ACE_Arg_Shifter shifter(argc, argv);

  while (shifter.is_anything_left())
  {
    const ACE_TCHAR* arg;

    if ((arg = shifter.get_the_parameter(ACE_TEXT("-ms"))) != 0)
    {
      minimum_separation.sec = ACE_OS::atoi(arg);
      shifter.consume_arg();
    }
    else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-reliable")) == 0)
    {
      reliable = true;
      shifter.consume_arg();
    }
    else
    {
      shifter.ignore_arg();
    }
  }
}

} // namespace

typedef std::pair<Foo, ACE_Time_Value> FooInfo;
typedef std::vector<FooInfo> Foos;
typedef std::map< ::CORBA::Long, Foos> SampleMap;

class DataReaderListenerImpl : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(unsigned int expected_num_samples)
  : valid_(true)
  , condition_(lock_)
  , num_samples_(0)
  , expected_num_samples_(expected_num_samples)
  {
  }

  virtual ~DataReaderListenerImpl()
  {
  }

  void on_data_available(DDS::DataReader_ptr reader)
  {
    try {
      FooDataReader_var message_dr =
        FooDataReader::_narrow(reader);

      if (CORBA::is_nil(message_dr.in())) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T %N:%l: on_data_available()")
          ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(-1);
      }

      FooSeq foos;
      DDS::SampleInfoSeq info;

      DDS::ReturnCode_t error = message_dr->take(foos, info, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      ACE_Time_Value now(ACE_OS::gettimeofday());

      ACE_GUARD(ACE_SYNCH_MUTEX, g, this->lock_);
      if (error == DDS::RETCODE_OK) {
        for (unsigned int i = 0; i < foos.length(); ++i) {
          const DDS::SampleInfo& si = info[i];
          if (si.valid_data) {
            const Foo& foo = foos[i];
            map_[foo.key].push_back(std::make_pair(foo, now));
            ++num_samples_;
          }
          else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: instance is disposed\n")));
          }
          else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: instance is unregistered\n")));

          }
          else {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("%T %N:%l: on_data_available()")
              ACE_TEXT(" ERROR: unknown instance state: %d\n"),
              si.instance_state));
          }
        }
      }
      else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T %N:%l: on_data_available()")
          ACE_TEXT(" ERROR: unexpected status: %d\n"),
          error));
      }

      if (complete()) {
        condition_.signal();
      }
    }
    catch (const CORBA::Exception& e) {
      e._tao_print_exception("Exception caught in on_data_available():");
      ACE_OS::exit(-1);
    }
  }

  void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
  {
    valid_ = false;
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%T %N:%l: on_requested_deadline_missed()")
      ACE_TEXT(" ERROR: should not occur\n")));
  }

  void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
  {
    valid_ = false;
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%T %N:%l: on_requested_incompatible_qos()")
      ACE_TEXT(" ERROR: should not occur\n")));
  }

  void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
  {
  }

  void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&)
  {
  }

  void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
  {
    valid_ = false;
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%T %N:%l: on_sample_rejected()")
      ACE_TEXT(" ERROR: should not occur\n")));
  }

  void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&)
  {
    valid_ = false;
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%T %N:%l: on_sample_lost()")
      ACE_TEXT(" ERROR: should not occur\n")));
  }

  void wait_for_completion()
  {
    ACE_GUARD(ACE_SYNCH_MUTEX, g, this->lock_);
    ACE_Time_Value start(ACE_OS::gettimeofday());
    ACE_Time_Value wait_time(180);
    do {
      condition_.wait(&wait_time);
    } while (!complete() && (ACE_OS::gettimeofday() - start >= wait_time));
  }

  SampleMap map_;
  bool valid_;

private:

  bool complete() const
  {
    return expected_num_samples_ >= num_samples_;
  }

  ACE_SYNCH_MUTEX lock_;
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;
  unsigned int num_samples_;
  const unsigned int expected_num_samples_;
};

void validate(const float expected_x, const float actual_x,
              const float expected_y, const float actual_y,
              const int key, bool& valid)
{
  if (expected_x != actual_x || expected_y != actual_y) {
    valid = false;
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%N:%l validate()")
      ACE_TEXT(" ERROR: for key %d received sample x=%f y=%f")
      ACE_TEXT("but expected x=%f y=%f!\n"),
      key, actual_x, actual_y, expected_x, expected_y));
  }
  else {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l validate() - ")
      ACE_TEXT("received message with key %d received sample x=%f y=%f")
      ACE_TEXT("but expected x=%f y=%f!\n"),
      key, actual_x, actual_y, expected_x, expected_y));
  }
}

bool verify_unreliable(const size_t expected_samples, SampleMap& rcvd_samples)
{
  bool valid = true;
  float LAST_SAMPLE_X = 0;

  for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j) {
    const Foos& foos = rcvd_samples[j];
    size_t seen = foos.size();

    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%N:%l verify_reliable() - ")
        ACE_TEXT("Instance %d messages received:\n"), j));
    for (size_t recvd = 0; recvd < foos.size(); recvd++)
    {
      const FooInfo& fooInfo = foos[recvd];
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("\tkey: %d x: %f y: %f\n"),
        fooInfo.first.key, fooInfo.first.x, fooInfo.first.y));
    }

    if (seen != expected_samples) {
      valid = false;
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l verify_unreliable()")
        ACE_TEXT(" ERROR: for key %d received %d sample(s), expected %d!\n"),
        j, seen, expected_samples));
    } else {
      for (size_t i = 0; i < expected_samples; ++i) {
        const FooInfo& fooInfo = foos[i];
        // NOTE: spec does not enforce ordering, but relying on the fact
        // that this is TCP and that the first sent
        // message will also be the first received message in the data reader
        validate(LAST_SAMPLE_X, fooInfo.first.x, 0.0f, fooInfo.first.y, j, valid);
        LAST_SAMPLE_X += (float)(NUM_INSTANCES * SAMPLES_PER_CYCLE);
      }
    }
    LAST_SAMPLE_X = (float)((j+1)*SAMPLES_PER_CYCLE);
  }
  return valid;
}

bool verify_reliable(const size_t expected_samples, SampleMap& rcvd_samples, SampleMap& sent_samples)
{
  bool valid = true;
  float LAST_SAMPLE_X = 0;

  for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j) {
    const Foos& foos = rcvd_samples[j];
    const Foos& sent_foos = sent_samples[j];
    size_t seen = foos.size();

    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%N:%l verify_reliable() - ")
        ACE_TEXT("Instance %d messages received:\n"), j));
    for (size_t recvd = 0; recvd < foos.size(); recvd++)
    {
      const FooInfo& fooInfo = foos[recvd];
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("\tkey: %d x: %f y: %f\n"),
        fooInfo.first.key, fooInfo.first.x, fooInfo.first.y));
    }

    // each delay should result in 2 samples being seen,
    // the first one and the last one in the filter window
    if (seen != expected_samples) {
      valid = false;
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l verify_reliable()")
        ACE_TEXT(" ERROR: for key %d received %d sample(s), expected %d!\n"),
        j, seen, expected_samples));
    } else {
        // each successive sample was sent with x = 0.0 to x = (SAMPLES_PER_CYCLE - 1)
        // NOTE: spec does not enforce ordering, but relying on the fact that
        // this is TCP and that the first and last sent message will also be
        // the first and last received message in the data reader
      const FooInfo* foo_info = &foos[0];
      validate(LAST_SAMPLE_X, foo_info->first.x, 0.0f, foo_info->first.y, foo_info->first.key, valid);
      ACE_Time_Value diff = foo_info->second - sent_foos[0].second;
      if (diff.sec() > minimum_separation.sec) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
        ACE_TEXT(" ERROR: Test setup incorrectly, sending and receiving samples should not take %d seconds!\n"),
        diff.sec()));
      }

      LAST_SAMPLE_X += SAMPLES_PER_CYCLE - 1;
      foo_info = &foos[1];
      validate(LAST_SAMPLE_X, foo_info->first.x, 0.0f, foo_info->first.y, foo_info->first.key, valid);

      diff = foo_info->second - foos[0].second;
      if (diff.sec() < (minimum_separation.sec * 0.75) ||
          diff.sec() > (minimum_separation.sec * 1.5)) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
          ACE_TEXT(" ERROR: The time between the 1st and 2nd sample should be about %d seconds, but it was %d seconds!\n"),
          minimum_separation.sec, diff.sec()));
      }

      LAST_SAMPLE_X += SAMPLES_PER_CYCLE;
      foo_info = &foos[2];
      validate(LAST_SAMPLE_X, foo_info->first.x, 1.0f, foo_info->first.y, foo_info->first.key, valid);

      diff = foo_info->second - foos[1].second;
      if (diff.sec() < (minimum_separation.sec * 0.75) ||
        diff.sec() > (minimum_separation.sec * 1.5)) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
          ACE_TEXT(" ERROR: The time between the 2nd and 3rd sample should be about %d seconds, but it was %d seconds!\n"),
          minimum_separation.sec, diff.sec()));
      }

      LAST_SAMPLE_X += (NUM_INSTANCES)*SAMPLES_PER_CYCLE + 1;
      foo_info = &foos[3];
      validate(LAST_SAMPLE_X, foo_info->first.x, 0.0f, foo_info->first.y, foo_info->first.key, valid);

      diff = foo_info->second - foos[2].second;
      if (diff.sec() < (minimum_separation.sec * 1.5) ||
          diff.sec() >= (minimum_separation.sec * 3)) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
          ACE_TEXT(" ERROR: The time between the 3rd and 4th sample should be about %d seconds, but it was %d seconds!\n"),
          SEPARATION_MULTIPLIER * minimum_separation.sec, diff.sec()));
      }

      LAST_SAMPLE_X += SAMPLES_PER_CYCLE -1;
      foo_info = &foos[4];
      validate(LAST_SAMPLE_X, foo_info->first.x, 0.0f, foo_info->first.y, foo_info->first.key, valid);

      diff = foo_info->second - foos[3].second;
      if (diff.sec() < (minimum_separation.sec * 0.75) ||
        diff.sec() > (minimum_separation.sec * 1.5)) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
          ACE_TEXT(" ERROR: The time between the 4th and 5th sample should be about %d seconds, but it was %d seconds!\n"),
          minimum_separation.sec, diff.sec()));
      }

      LAST_SAMPLE_X += SAMPLES_PER_CYCLE;
      foo_info = &foos[5];
      validate(LAST_SAMPLE_X, foo_info->first.x, 1.0f, foo_info->first.y, foo_info->first.key, valid);

      diff = foo_info->second - foos[4].second;
      if (diff.sec() < (minimum_separation.sec * 0.75) ||
        diff.sec() > (minimum_separation.sec * 1.5)) {
        valid = false;
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%N:%l verify_reliable()")
          ACE_TEXT(" ERROR: The time between the 5th and 6th sample should be about %d seconds, but it was %d seconds!\n"),
          minimum_separation.sec, diff.sec()));
      }
    }
    LAST_SAMPLE_X = (float)((NUM_INSTANCES-j)*SAMPLES_PER_CYCLE + j*SAMPLES_PER_CYCLE);
  }
  return valid;
}

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  parse_args(argc, argv);

  if (minimum_separation.sec < 1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l main()")
                      ACE_TEXT(" ERROR: minimum_separation must be non-zero!\n")), -1);
  }

  bool valid = true;
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create Participant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_participant failed!\n")), -1);
    }

    // Create Subscriber
    DDS::Subscriber_var subscriber =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
        DDS::SubscriberListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(subscriber.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_subscriber failed!\n")), -1);
    }

    // Create Publisher
    DDS::Publisher_var publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_publisher failed!\n")), -1);
    }

    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: register_type failed!\n")), -1);
    }

    // Create Topic (FooTopic)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
        type_name,
        TOPIC_QOS_DEFAULT,
        DDS::TopicListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_topic failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderQos dr_qos;

    if (subscriber->get_default_datareader_qos(dr_qos) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);
    }

    dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    dr_qos.time_based_filter.minimum_separation = minimum_separation;

    if (reliable) {
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      dr_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = 50;
    }

    const size_t EXPECTED_ITERATIONS = 2;
    const size_t TOTAL_SAMPLES_PER_INSTANCE = (reliable ? 3 : 1) * EXPECTED_ITERATIONS;

    DataReaderListenerImpl* listener_impl = new DataReaderListenerImpl(TOTAL_SAMPLES_PER_INSTANCE * NUM_INSTANCES);
    DDS::DataReaderListener_var listener = listener_impl;

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
        dr_qos,
        listener,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);
    }

    FooDataReader_var reader_i = FooDataReader::_narrow(reader);
    if (CORBA::is_nil(reader_i)) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: _narrow failed!\n")), -1);
    }

    DDS::DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    if (reliable) {
      dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time.sec = 1;
      dw_qos.reliability.max_blocking_time.nanosec = 0;
      dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.resource_limits.max_samples = 100;
      dw_qos.resource_limits.max_samples_per_instance = 50;
    }

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic.in(),
        dw_qos,
        DDS::DataWriterListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datawriter failed!\n")), -1);
    }

    FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
    if (CORBA::is_nil(writer_i)) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: _narrow failed!\n")), -1);
    }

    // Block until Subscriber is associated
    DDS::StatusCondition_var cond = writer->get_statuscondition();
    cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(cond);

    DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    do {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: wait failed!\n")), -1);
      }

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: Failed to get publication match status!\n")), -1);
      }
    } while (matches.current_count < 1);

    ws->detach_condition(cond);

    //
    // Verify TIME_BASED_FILTER is properly filtering samples.
    // We write a number of samples over a finite period of
    // time, and then verify we receive the expected number
    // of samples.
    //

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%N:%l main()")
      ACE_TEXT(" INFO: Testing %d second minimum separation...\n"),
      minimum_separation.sec));

    SampleMap send_map;

    // We expect to receive up to one sample per
    // cycle (all others should be filtered).
    size_t global_sent_msg_counter = 0;
    for (size_t i = 0; i < EXPECTED_ITERATIONS; ++i) {
      for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j) {
        Foo foo = { j, 0, 0, 0 }; // same instance required for repeated samples
        const ACE_Time_Value start = ACE_OS::gettimeofday();
        for (size_t k = 0; k < SAMPLES_PER_CYCLE; ++k) {
          foo.x = (CORBA::Float)global_sent_msg_counter++;

          send_map[foo.key].push_back(std::make_pair(foo, ACE_OS::gettimeofday ()));
          if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("%N:%l main()")
              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
          }
          else {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l main() - ")
              ACE_TEXT("wrote sample with key: %d and x: %f y: %f \n"), foo.key, foo.x, foo.y));
          }
        }

        if (reliable) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l main() - ")
            ACE_TEXT("reliable, so waiting minimum_separation + 1 = %d seconds.\n"), minimum_separation.sec + 1));
          ACE_OS::sleep(minimum_separation.sec + 1);
          foo.y = 1.0f;
          for (size_t k = 0; k < SAMPLES_PER_CYCLE; ++k) {
            foo.x = (CORBA::Float)global_sent_msg_counter++;
            send_map[foo.key].push_back(std::make_pair(foo, ACE_OS::gettimeofday ()));
            if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
              ACE_ERROR_RETURN((LM_ERROR,
                ACE_TEXT("%N:%l main()")
                ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
            }
            else {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l main() - ")
                ACE_TEXT("wrote sample with key: %d and x: %f y: %f \n"), foo.key, foo.x, foo.y));
            }
          }
        }
      }

      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l main() - ")
        ACE_TEXT("waiting iteration separation time - %d seconds.\n"),
        SEPARATION_MULTIPLIER * minimum_separation.sec));
      ACE_OS::sleep(SEPARATION_MULTIPLIER * minimum_separation.sec);
    }

    listener_impl->wait_for_completion();
    if (reliable) {
      valid = verify_reliable(TOTAL_SAMPLES_PER_INSTANCE, listener_impl->map_, send_map);
    } else {
      valid = verify_unreliable(TOTAL_SAMPLES_PER_INSTANCE, listener_impl->map_);
    }
    valid &= listener_impl->valid_;

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("Caught in main()");
    valid = false;
  }

  return valid ? 0 : -1;
}
