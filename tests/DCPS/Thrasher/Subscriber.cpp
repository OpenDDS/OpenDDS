/*
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

namespace
{
  std::size_t expected_samples = 1024;
  std::size_t received_samples = 0;
  int n_publishers = 1;

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
  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    parse_args(argc, argv);

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> SUBSCRIBER STARTED\n")));

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
    { // Scope for contained entities
      // Create Subscriber
      DDS::Subscriber_var subscriber =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
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

      if (CORBA::is_nil(topic.in()))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" create_topic failed!\n")), 6);

      // Create DataReader
      ProgressIndicator progress =
        ProgressIndicator("(%P|%t)    SUBSCRIBER %d%% (%d samples received)\n",
                          expected_samples);

      DDS::DataReaderListener_var listener =
        new DataReaderListenerImpl(received_samples, progress);

      DDS::DataReaderQos reader_qos;
      subscriber->get_default_datareader_qos(reader_qos);

      reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
#endif

      DDS::DataReader_var reader =
        subscriber->create_datareader(topic.in(),
                                      reader_qos,
                                      listener.in(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(reader.in()))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" create_datareader failed!\n")), 7);

      // Block until Publisher completes
      DDS::StatusCondition_var cond = reader->get_statuscondition();
      cond->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(cond);

      DDS::Duration_t timeout =
        { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

      DDS::ConditionSeq conditions;
      DDS::SubscriptionMatchedStatus matches = {0, 0, 0, 0, 0};
      do
      {
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" wait failed!\n")), 8);

        if (reader->get_subscription_matched_status(matches) != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
            "ERROR: failed to get subscription matched status\n"));
          return 1;
        }
      }
      while (matches.current_count > 0 || matches.total_count < n_publishers);
      ws->detach_condition(cond);

    } // End scope for contained entities

    // Clean-up!
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- SUBSCRIBER PARTICIPANT DEL CONT ENTITIES\n")));
    participant->delete_contained_entities();
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- SUBSCRIBER DELETE PARTICIPANT\n")));
    dpf->delete_participant(participant.in());

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- SUBSCRIBER SHUTDOWN\n")));
    TheServiceParticipant->shutdown();
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- SUBSCRIBER VARS GOING OUT OF SCOPE\n")));
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in main()");
    return 9;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- SUBSCRIBER FINISHED\n")));

  if (received_samples != expected_samples) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) ERROR: subscriber - ")
      ACE_TEXT("received %d of expected %d samples.\n"),
      received_samples,
      expected_samples
    ));
    return 10;
  }

  return 0;
}

