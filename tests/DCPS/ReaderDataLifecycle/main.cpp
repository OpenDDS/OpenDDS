/*
 * $Id$
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
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>

#include "FooTypeTypeSupportImpl.h"

#ifdef ACE_AS_STATIC_LIBS
# include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

namespace
{
static OpenDDS::DCPS::TransportIdType transportId = 0;

static const size_t SAMPLES_PER_TEST = 100;

static const DDS::Duration_t autopurge_delay = { 5, 0 };

} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try
  {
    TheParticipantFactoryWithArgs(argc, argv);

    // Create Participant
    DDS::DomainParticipant_var participant =
      TheParticipantFactory->create_participant(42,
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

    // Attach Subscriber Transport
    ++transportId;

    OpenDDS::DCPS::TransportConfiguration_rch sub_config =
      TheTransportFactory->get_or_create_configuration(transportId,
                                                       ACE_TEXT("SimpleTcp"));

    OpenDDS::DCPS::TransportImpl_rch sub_transport =
      TheTransportFactory->create_transport_impl(transportId);

    OpenDDS::DCPS::AttachStatus sub_status =
      sub_transport->attach(subscriber.in());

    if (sub_status != OpenDDS::DCPS::ATTACH_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: attach_transport failed!\n")), -1);
    }

    // Attach Publisher Transport
    ++transportId;

    OpenDDS::DCPS::TransportConfiguration_rch pub_config =
      TheTransportFactory->get_or_create_configuration(transportId,
                                                       ACE_TEXT("SimpleTcp"));

    OpenDDS::DCPS::TransportImpl_rch pub_transport =
      TheTransportFactory->create_transport_impl(transportId);

    OpenDDS::DCPS::AttachStatus pub_status =
      pub_transport->attach(publisher.in());

    if (pub_status != OpenDDS::DCPS::ATTACH_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: attach_transport failed!\n")), -1);
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
    DDS::DataReaderQos reader_qos;

    if (subscriber->get_default_datareader_qos(reader_qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: get_default_datareader_qos failed!\n")), -1);
    }

    reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = autopurge_delay;
    reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = autopurge_delay;

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
    // Test autopurge_nowriter_samples_delay and
    // autopurge_disposed_samples_delay facets of
    // the READER_DATA_LIFECYCLE QoS policy.
    //
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing autopurge_disposed_samples_delay...\n")));
    {
      Foo foo = { 42, 0, 0, 0 };

      DDS::InstanceHandle_t handle = writer_i->register_instance(foo);

      for (size_t i = 0; i < SAMPLES_PER_TEST; ++i)
      {
        if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
        }
      }
      ACE_OS::sleep(5); // wait for samples to arrive

      writer_i->dispose(foo, handle);

      ACE_OS::sleep(10); // wait for dispose and autopurge

      // Verify that no samples are available
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
                            ACE_TEXT(" ERROR: Unexpected samples taken!\n")), -1);
        }
      }
    }

    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing autopurge_nowriter_samples_delay...\n")));
    {
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

      publisher->delete_datawriter(writer);

      ACE_OS::sleep(10); // wait for disassociation and autopurge

      // Verify that no samples are available
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
                            ACE_TEXT(" ERROR: Unexpected samples taken!\n")), -1);
        }
      }
    }

    // Clean-up!
    participant->delete_contained_entities();
    TheParticipantFactory->delete_participant(participant);

    TheTransportFactory->release();
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("Caught in main()");
    return -1;
  }

  return 0;
}
