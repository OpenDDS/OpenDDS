/*
 * $Id$
 */

#include <ace/Arg_Shifter.h>
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

static const CORBA::Octet FIRST_SAMPLE = 0xbe;
static const CORBA::Octet SECOND_SAMPLE = 0xef;

static bool use_source_timestamp = false;

void
parse_args(int& argc, ACE_TCHAR** argv)
{
  ACE_Arg_Shifter shifter(argc, argv);

  while (shifter.is_anything_left())
  {
    if (shifter.cur_arg_strncasecmp(ACE_TEXT("source")) == 0)
    {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("%N:%l parse_args()")
                 ACE_TEXT(" INFO: using SOURCE_TIMESTAMP\n")));
      use_source_timestamp = true;
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

    OpenDDS::DCPS::SubscriberImpl* subscriber_i =
      dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>(subscriber.in());

    if (subscriber_i == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: dynamic_cast failed!\n")), -1);
    }

    OpenDDS::DCPS::AttachStatus sub_status =
      subscriber_i->attach_transport(sub_transport.in());

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

    OpenDDS::DCPS::PublisherImpl* publisher_i =
      dynamic_cast<OpenDDS::DCPS::PublisherImpl*>(publisher.in());

    if (publisher_i == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: dynamic_cast failed!\n")), -1);
    }

    OpenDDS::DCPS::AttachStatus pub_status =
      publisher_i->attach_transport(pub_transport.in());

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
    DDS::DataReaderQos qos;

    if (subscriber->get_default_datareader_qos(qos) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);
    }

    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    if (use_source_timestamp)
    {
      qos.destination_order.kind =
        DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    }

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
                                    qos,
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
                          ACE_TEXT(" ERROR: failed to get publication match status!\n")), -1);
      }
    }
    while (matches.current_count < 1);

    ws->detach_condition(cond);

    //
    // FooDataReader take/read operations should return
    // samples within the same instance ordered by either
    // source or reception timestamp.
    //
    DDS::Time_t t1 = { 10, 0 };
    DDS::Time_t t2 = {  0, 0 };

    Foo f1 = { 0, 0, 0, FIRST_SAMPLE  /* o77 */ };
    Foo f2 = { 0, 0, 0, SECOND_SAMPLE /* o77 */ };

    DDS::InstanceHandle_t instance =
      writer_i->register_instance(f1);

    if (writer_i->write_w_timestamp(f1, instance, t1) != DDS::RETCODE_OK)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: unable to write sample!\n")), -1);
    }
    if (writer_i->write_w_timestamp(f2, instance, t2) != DDS::RETCODE_OK)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: unable to write sample!\n")), -1);
    }

    ACE_OS::sleep(5); // wait for samples to arrive

    FooSeq foo;
    DDS::SampleInfoSeq info;
    instance = reader_i->lookup_instance( f1);

    if (reader_i->take_instance(foo,
                                info,
                                1,
                                instance,
                                DDS::ANY_SAMPLE_STATE,
                                DDS::ANY_VIEW_STATE,
                                DDS::ANY_INSTANCE_STATE) != DDS::RETCODE_OK)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: unable to take next sample!\n")), -1);
    }

    // Verify first sample is ordered correctly
    if ((use_source_timestamp && foo[0].o77 != SECOND_SAMPLE) ||
        (!use_source_timestamp && foo[0].o77 != FIRST_SAMPLE))
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: DESTINATION_ORDER is incorrect!\n")), 1);
    }

    // Clean-up!
    participant->delete_contained_entities();
    TheParticipantFactory->delete_participant(participant);

    TheTransportFactory->release();
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in main()");
    return -1;
  }

  return 0;
}
