/*
 */

#include <ace/Log_Msg.h>

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

namespace
{

static const size_t SAMPLES_PER_TEST = 100;

} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
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
    DDS::SubscriberQos subscriber_qos;
    if (participant->get_default_subscriber_qos(subscriber_qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: get_qos failed!\n")), -1);
    }

    // TODO Currently only TOPIC access_scope is supported.
    subscriber_qos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
    subscriber_qos.presentation.coherent_access = true;
    subscriber_qos.presentation.ordered_access = true;

    DDS::Subscriber_var subscriber =
      participant->create_subscriber(subscriber_qos,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(subscriber.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber failed!\n")), -1);
    }

    // Create Publisher
    DDS::PublisherQos publisher_qos;
    if (participant->get_default_publisher_qos(publisher_qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: get_qos failed!\n")), -1);
    }

    // TODO Currently only TOPIC access_scope is supported.
    publisher_qos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
    publisher_qos.presentation.coherent_access = true;
    publisher_qos.presentation.ordered_access = true;

    DDS::Publisher_var publisher =
      participant->create_publisher(publisher_qos,
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
                                CORBA::String_var(ts->get_type_name()),
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
    DDS::DataReaderQos reader_qos;

    if (subscriber->get_default_datareader_qos(reader_qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: get_default_datareader_qos failed!\n")), -1);
    }

    reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
                                    reader_qos,
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

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic.in(),
                                   DATAWRITER_QOS_DEFAULT,
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
    }
    while (matches.current_count < 1);

    ws->detach_condition(cond);

    //
    // Test coherent_access and ordered_access facets of
    // the PRESENTATION QoS policy.
    //
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing coherent_access...\n")));
    {
      publisher->begin_coherent_changes();

      for (size_t i = 0; i < SAMPLES_PER_TEST; ++i)
      {
        Foo foo;
        if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
        }
      }
      ACE_OS::sleep(5); // wait for samples to arrive

      // Verify that no samples are yet available
      {
        FooSeq foo;
        DDS::SampleInfoSeq info;

        DDS::ReturnCode_t error;
        if ((error = reader_i->take(foo,
                                    info,
                                    DDS::LENGTH_UNLIMITED,
                                    DDS::ANY_SAMPLE_STATE,
                                    DDS::ANY_VIEW_STATE,
                                    DDS::ANY_INSTANCE_STATE)) != DDS::RETCODE_NO_DATA)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unexpected samples read!\n")), -1);
        }
      }

      publisher->end_coherent_changes();
      ACE_OS::sleep(5); // wait for samples to arrive

      //Verify coherent samples are now available
      {
        FooSeq foo;
        DDS::SampleInfoSeq info;

        DDS::ReturnCode_t error;
        if ((error = reader_i->take(foo,
                                    info,
                                    DDS::LENGTH_UNLIMITED,
                                    DDS::ANY_SAMPLE_STATE,
                                    DDS::ANY_VIEW_STATE,
                                    DDS::ANY_INSTANCE_STATE)) != DDS::RETCODE_OK)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unexpected number of samples taken!\n")), -1);
        }

        if (foo.length() != SAMPLES_PER_TEST)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unexpected number of samples taken!\n")), -1);
        }
      }
    }

    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing ordered_access...\n")));
    {
      for (size_t i = 0; i < SAMPLES_PER_TEST / 2; ++i)
      {
        // Write first instance
        Foo foo1 = { 0, 0, 0, 0 };
        if (writer_i->write(foo1, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
        }

        // Write second instance
        Foo foo2 = { 42, 0, 0, 0 };
        if (writer_i->write(foo2, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
        }
      }
      ACE_OS::sleep(5); // wait for samples to arrive

      // Verify samples are sorted in source order
      {
        FooSeq foo;
        DDS::SampleInfoSeq info;

        DDS::ReturnCode_t error;
        if ((error = reader_i->take(foo,
                                    info,
                                    DDS::LENGTH_UNLIMITED,
                                    DDS::ANY_SAMPLE_STATE,
                                    DDS::ANY_VIEW_STATE,
                                    DDS::ANY_INSTANCE_STATE)) != DDS::RETCODE_OK)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unable to take samples!\n")), -1);
        }

        DDS::Time_t last_timestamp = { 0, 0 };
        for (CORBA::ULong i = 0; i < info.length(); ++i)
        {
          using OpenDDS::DCPS::operator<;
          if (info[i].source_timestamp < last_timestamp)
          {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Samples taken out of order!\n")), -1);
          }
          last_timestamp = info[i].source_timestamp;
        }
      }
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("Caught in main()");
    return -1;
  }

  return 0;
}
