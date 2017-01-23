/*
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsTopicC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>

#include "SubscriberListenerImpl.h"
#include "FooTypeTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"

namespace
{

  ACE_CString toStr(int n)
  {
    char buf[20];
    return ACE_OS::itoa(n, buf, 10);
  }

  std::size_t expected_samples = 1;
  std::size_t received_samples = 0;
  std::size_t missed_samples = 0;

  int n_publishers = 1;
  std::size_t samples_per_cycle = 1;
  int sample_count_sleep_msec = 250;
  int delay_between_cycles_msec = 250;
  int deadline_msec = 250;
  bool use_cft = false;

  void
  parse_args(int& argc, ACE_TCHAR** argv)
  {
    ACE_Arg_Shifter shifter(argc, argv);

    while (shifter.is_anything_left())
    {
      const ACE_TCHAR* arg;

      if ((arg = shifter.get_the_parameter(ACE_TEXT("-n"))))
      {
        expected_samples = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-t"))))
      {
        n_publishers = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-c"))))
      {
        samples_per_cycle = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-s"))))
      {
        sample_count_sleep_msec = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-d"))))
      {
        delay_between_cycles_msec = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-l"))))
      {
        deadline_msec = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-f"))))
      {
        use_cft = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else
      {
        shifter.ignore_arg();
      }
    }
  }
} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  parse_args(argc, argv);

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> SUBSCRIBER STARTED\n")));

  ::CORBA::Long sec = deadline_msec / 1000;
  ::CORBA::ULong remainder_msec = (deadline_msec - 1000*sec);
  ::CORBA::ULong nanosec = remainder_msec * 1000000;

  DDS::Duration_t const DEADLINE_PERIOD =
    {
      sec,
      nanosec
    };

  bool deadline_used = DEADLINE_PERIOD.sec > 0 || DEADLINE_PERIOD.nanosec > 0;

  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    SubscriberListenerImpl * subscriberListener =
      new SubscriberListenerImpl(received_samples, missed_samples);

    DDS::SubscriberListener_var subscriberListener_var = subscriberListener;

    // Create Participant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" create_participant failed!\n")), 1);

    ACE_Time_Value delay_between_cycles(0, delay_between_cycles_msec * 1000);

    bool expected_samples_received = false;
    int i = 0;

    do
      {
        ++i;

        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> Subscriber cycle %d\n"), i));

        // Create Subscriber
        DDS::Subscriber_var subscriber =
          participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                         subscriberListener,
                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(subscriber.in()))
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" create_subscriber failed!\n")), 2);

        // Register Type (FooType)
        FooTypeSupport_var ts = new FooTypeSupportImpl;
        if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" register_type failed!\n")), 5);

        // Create Topic (FooTopic)
        DDS::Topic_var topic =
          participant->create_topic("FooTopic",
                                    CORBA::String_var(ts->get_type_name()),
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        DDS::TopicDescription_ptr topic_used = topic.in();
        DDS::ContentFilteredTopic_ptr cft = 0;

        if (use_cft)
          {
            // Topic name must be unique.
            ACE_CString topic_name = "FooTopic-Filtered-" + toStr(i);
            cft =
              participant->create_contentfilteredtopic(topic_name.c_str(),
                                                       topic,
                                                       "key > 0",
                                                       DDS::StringSeq());
            if (CORBA::is_nil(cft))
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("%N:%l: main()")
                                ACE_TEXT(" create_contentfilteredtopic failed!\n")), 8);

            topic_used = cft;
          }

        if (CORBA::is_nil(topic.in()))
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" create_topic failed!\n")), 6);

        // Create DataReader

        DDS::DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);

        reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        if (deadline_used)
          {
            reader_qos.deadline.period.sec     = DEADLINE_PERIOD.sec;
            reader_qos.deadline.period.nanosec = DEADLINE_PERIOD.nanosec;
          }

        DDS::DataReader_var reader =
          subscriber->create_datareader(topic_used,
                                        reader_qos,
                                        DDS::DataReaderListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(reader.in()))
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" create_datareader failed!\n")), 7);


        ACE_Time_Value sample_count_sleep(0, sample_count_sleep_msec * 1000);
        std::size_t sample_count;
        std::size_t sample_count_start = subscriberListener->samples_processed();
        do
          {
            ACE_OS::sleep(sample_count_sleep);
            sample_count =
              subscriberListener->samples_processed();
            expected_samples_received = sample_count >= expected_samples;
            // ACE_DEBUG((LM_DEBUG, "(%P|%t) sample_count = %d\n", sample_count));
          }
        while (!expected_samples_received &&
               (sample_count - sample_count_start) < samples_per_cycle);

        subscriber->delete_datareader(reader.in());

        if (use_cft)
          CORBA::release(cft);

        participant->delete_subscriber(subscriber.in());

        ACE_OS::sleep(delay_between_cycles);
      }
    while (!expected_samples_received);

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();

    ACE_DEBUG ((LM_INFO,
                ACE_TEXT("INFO: %d samples received\n"),
                subscriberListener->received_samples()));
    if (deadline_used)
      ACE_DEBUG ((LM_INFO,
                  ACE_TEXT("INFO: deadline missed %d times\n"),
                  subscriberListener->missed_samples()));

  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in main()");
    return 9;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- SUBSCRIBER FINISHED\n")));

  return 0;
}
