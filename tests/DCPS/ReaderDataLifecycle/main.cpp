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

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "dds/DCPS/StaticIncludes.h"

namespace {

  const size_t SAMPLES_PER_TEST = 100;
  const DDS::Duration_t autopurge_delay = { 5, 0 };

  struct Cleanup
  {
    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;
    DDS::WaitSet_var ws;
    DDS::StatusCondition_var cond;
    ~Cleanup()
    {
      rem_wait_set();
      // Clean-up!
      participant->delete_contained_entities();
      dpf->delete_participant(participant);

      TheServiceParticipant->shutdown();
    }

    void add_wait_set(DDS::StatusCondition_var condition)
    {
      cond = condition;
      ws = new DDS::WaitSet;
      ws->attach_condition(cond);
    }

    void rem_wait_set()
    {
      if (!CORBA::is_nil(ws))
        ws->detach_condition(cond);
    }
  };
} // namespace

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Cleanup cu;
    cu.dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create Participant
    cu.participant =
      cu.dpf->create_participant(42,
                                 PARTICIPANT_QOS_DEFAULT,
                                 DDS::DomainParticipantListener::_nil(),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(cu.participant)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")), -1);
    }

    // Create Subscriber
    DDS::Subscriber_var subscriber =
      cu.participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(subscriber)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber failed!\n")), -1);
    }

    // Create Publisher
    DDS::Publisher_var publisher =
      cu.participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")), -1);
    }

    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(cu.participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")), -1);
    }

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      cu.participant->create_topic("FooTopic",
                                   CORBA::String_var(ts->get_type_name()),
                                   TOPIC_QOS_DEFAULT,
                                   DDS::TopicListener::_nil(),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderQos reader_qos;
    if (subscriber->get_default_datareader_qos(reader_qos) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: get_default_datareader_qos failed!\n")), -1);
    }

    const DDS::Duration_t default_delay =
      reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay;

    reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = autopurge_delay;

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic,
                                    reader_qos,
                                    DDS::DataReaderListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader)) {
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

    DDS::DataWriterQos writer_qos;
    publisher->get_default_datawriter_qos(writer_qos);
    writer_qos.writer_data_lifecycle.autodispose_unregistered_instances = false;

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic,
                                   writer_qos,
                                   DDS::DataWriterListener::_nil(),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer)) {
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

    cu.add_wait_set(cond);

    const DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    while (true) {
      if (writer->get_publication_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: Failed to get publication match status!\n")), -1);
      }

      if (matches.current_count > 0) {
        break;
      }

      if (cu.ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: wait failed!\n")), -1);
      }
    }
    cu.rem_wait_set();

    //
    // Test autopurge_nowriter_samples_delay and
    // autopurge_disposed_samples_delay facets of
    // the READER_DATA_LIFECYCLE QoS policy.
    //
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing autopurge_disposed_samples_delay...\n")));
    {
      const Foo foo = { 42, 0, 0, 0 };

      DDS::InstanceHandle_t handle = writer_i->register_instance(foo);

      for (size_t i = 0; i < SAMPLES_PER_TEST; ++i) {
        if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: Unable to write sample!\n")), -1);
        }
      }

      ACE_OS::sleep(5); // wait for samples to arrive

      writer_i->dispose(foo, handle);
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" disposed\n")));

      ACE_OS::sleep(10); // wait for dispose and autopurge

      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" take\n")));
      // Verify that no samples are available
      {
        FooSeq foo;
        DDS::SampleInfoSeq info;
        const DDS::ReturnCode_t error = reader_i->take(foo,
                                                       info,
                                                       DDS::LENGTH_UNLIMITED,
                                                       DDS::ANY_SAMPLE_STATE,
                                                       DDS::ANY_VIEW_STATE,
                                                       DDS::ANY_INSTANCE_STATE);
        if (error != DDS::RETCODE_NO_DATA) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unexpected return code for take %d!\n"),
                            error), -1);
        }
      }
    }

    ACE_DEBUG((LM_INFO,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" INFO: Testing autopurge_nowriter_samples_delay...\n")));
    reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = default_delay;
    reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = autopurge_delay;
    reader->set_qos(reader_qos);

    {
      const Foo foo = { 43, 0, 0, 0 };

      for (size_t i = 0; i < SAMPLES_PER_TEST; ++i) {
        if (writer_i->write(foo, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
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
        const DDS::ReturnCode_t error = reader_i->take(foo,
                                                       info,
                                                       DDS::LENGTH_UNLIMITED,
                                                       DDS::ANY_SAMPLE_STATE,
                                                       DDS::ANY_VIEW_STATE,
                                                       DDS::ANY_INSTANCE_STATE);
        if (error != DDS::RETCODE_NO_DATA) {
          for (CORBA::ULong i = 0; i < info.length(); ++i) {
            ACE_DEBUG((LM_DEBUG, "sample %d instance %d state %d key %d\n",
              i, info[i].instance_handle, info[i].instance_state,
              info[i].valid_data ? foo[i].key : -1234));
          }
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: Unexpected samples taken!\n")), -1);
        }
      }
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Caught in main()");
    return -1;
  }

  return 0;
}
