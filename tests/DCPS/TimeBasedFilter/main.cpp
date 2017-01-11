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

#include "dds/DCPS/StaticIncludes.h"
#include "ace/OS_NS_unistd.h"

namespace
{
static DDS::Duration_t minimum_separation = { 5, 0 };
static bool reliable = false;

static const size_t NUM_INSTANCES = 2;
static const size_t SAMPLES_PER_CYCLE = 5;

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
    else if (strcmp(shifter.get_current(), (ACE_TEXT("-r"))) == 0)
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

typedef std::pair<Foo, DDS::SampleInfo> FooInfo;
typedef std::vector<FooInfo> Foos;
typedef std::map< ::CORBA::Long, Foos> SampleMap;

bool verify_unreliable(FooDataReader_var reader_i, const size_t expected_samples, SampleMap& samples)
{
  bool valid = true;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%N:%l main()")
    ACE_TEXT(" INFO: Expecting %d instances...\n"),
    NUM_INSTANCES));
  for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j)
  {
    const Foos& foos = samples[j];
    size_t seen = foos.size();
    if (seen != expected_samples)
    {
      valid = false;
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: for key %d received %d sample(s), expected %d!\n"),
        j, seen, expected_samples));
    }
    else
    {
      for (size_t i = 0; i < expected_samples; ++i)
      {
        const FooInfo& fooInfo = foos[i];
        if (fooInfo.first.x != 0.0f)
        {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("%N:%l main()")
            ACE_TEXT(" ERROR: for key %d received x=%f, expected 0.0!\n"),
            j, fooInfo.first.x));
        }
      }
    }
  }
  return valid;
}


bool verify_reliable(FooDataReader_var reader_i, const size_t expected_samples, SampleMap& samples)
{
  bool valid = true;
  for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j)
  {
    const Foos& foos = samples[j];
    size_t seen = foos.size();
    // each delay should result in 2 samples being seen, the first one and the last one in the filter window
    if (seen != expected_samples * 2)
    {
      valid = false;
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: for key %d received %d sample(s), expected %d!\n"),
        j, seen, expected_samples));
    }
    else
    {
      for (size_t i = 0; i < expected_samples * 2; ++i)
      {
        const FooInfo& fooInfo = foos[i];
        // each successive sample was sent with x = 0.0 to x = (SAMPLES_PER_CYCLE - 1)
        const float expected = (i % 2) == 0 ? 0.0f : (float)(SAMPLES_PER_CYCLE - 1);
        if (fooInfo.first.x != expected)
        {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("%N:%l main()")
            ACE_TEXT(" ERROR: for key %d received x=%f, expected %f!\n"),
            j, fooInfo.first.x, expected));
        }
      }
    }
  }
  return valid;
}

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  parse_args(argc, argv);

  if (minimum_separation.sec < 1)
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l main()")
                      ACE_TEXT(" ERROR: minimum_separation must be non-zero!\n")), -1);
  }

  bool valid = true;
  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create Participant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_participant failed!\n")), -1);
    }

    // Create Subscriber
    DDS::Subscriber_var subscriber =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
        DDS::SubscriberListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(subscriber.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_subscriber failed!\n")), -1);
    }

    // Create Publisher
    DDS::Publisher_var publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
        DDS::PublisherListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_publisher failed!\n")), -1);
    }

    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: register_type failed!\n")), -1);
    }

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
        ts->get_type_name(),
        TOPIC_QOS_DEFAULT,
        DDS::TopicListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_topic failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderQos dr_qos;

    if (subscriber->get_default_datareader_qos(dr_qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);
    }

    dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    dr_qos.time_based_filter.minimum_separation = minimum_separation;

    if (reliable)
    {
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      dr_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = 50;
    }

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
        dr_qos,
        DDS::DataReaderListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);
    }

    FooDataReader_var reader_i = FooDataReader::_narrow(reader);
    if (CORBA::is_nil(reader_i))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: _narrow failed!\n")), -1);
    }

    DDS::DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    if (reliable)
    {
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

    if (CORBA::is_nil(writer.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: create_datawriter failed!\n")), -1);
    }

    FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
    if (CORBA::is_nil(writer_i))
    {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("%N:%l main()")
        ACE_TEXT(" ERROR: _narrow failed!\n")), -1);
    }

    // Block until Subscriber is associated
    DDS::StatusCondition_var cond = writer->get_statuscondition();
    cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(cond);

    DDS::Duration_t timeout =
    { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    do
    {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: wait failed!\n")), -1);
      }

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
      {
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

    const size_t EXPECTED_SAMPLES = reliable ? 5 : 2;
    const size_t SEPARATION_MULTIPLIER = reliable ? 4 : 2;

    // We expect to receive up to one sample per
    // cycle (all others should be filtered).
    for (size_t i = 0; i < EXPECTED_SAMPLES; ++i)
    {
      for (::CORBA::Long j = 0; j < NUM_INSTANCES; ++j)
      {
        for (size_t k = 0; k < SAMPLES_PER_CYCLE; ++k)
        {
          ::CORBA::Float x = (CORBA::Float)k;
          Foo foo = { j, x, 0, 0 }; // same instance required for repeated samples
          if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
          {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("%N:%l main()")
              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
          }
        }
      }

      // Wait for at least two minimum_separation cycles
      ACE_OS::sleep(SEPARATION_MULTIPLIER * minimum_separation.sec);
    }

    SampleMap samples;
    for (;;)
    {
      Foo foo;
      DDS::SampleInfo info;

      DDS::ReturnCode_t error = reader_i->take_next_sample(foo, info);
      if (error == DDS::RETCODE_OK)
      {
        if (info.valid_data) {
          samples[foo.key].push_back(std::make_pair(foo, info));
        }
      }
      else if (error == DDS::RETCODE_NO_DATA)
      {
        break; // done!
      }
      else
      {
        ACE_ERROR_RETURN((
          LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: Unable to take sample, error %d!\n"), error), -1);
      }
    }

    if (!reliable)
    {
      valid = verify_unreliable(reader_i, EXPECTED_SAMPLES, samples);
    }
    else
    {
      valid = verify_reliable(reader_i, EXPECTED_SAMPLES, samples);
    }

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
