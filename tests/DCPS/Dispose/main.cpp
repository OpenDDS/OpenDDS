/*
 */

#include <ace/Log_Msg.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/SubscriptionInstance.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/DataReaderImpl.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>
#include <ace/OS_NS_unistd.h>

#include "FooTypeTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"

class DDS_TEST
{
public:
  DDS_TEST(FooDataReader_var reader)
    : reader_(reader)
  {
    impl_ = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader.in());
    if (impl_ == 0)
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: DDS_TEST()")
                 ACE_TEXT(" ERROR: dynamic_cast failed!\n")));
  }

  bool
  has_instance(DDS::InstanceHandle_t handle)
  {
    return this->impl_->instances_.find(handle)
      != this->impl_->instances_.end();
  }

  bool
  take_all_samples()
  {
    FooSeq foo;
    DDS::SampleInfoSeq si;

    return reader_->take(foo,
                         si,
                         DDS::LENGTH_UNLIMITED,
                         DDS::ANY_SAMPLE_STATE,
                         DDS::ANY_VIEW_STATE,
                         DDS::ANY_INSTANCE_STATE) == DDS::RETCODE_OK;
  }

  bool operator!()
  {
    return impl_ == 0;
  }

private:
  FooDataReader_var reader_;
  OpenDDS::DCPS::DataReaderImpl* impl_;
};

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
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")), 1);

    // Create Subscriber
    DDS::Subscriber_var subscriber =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(subscriber.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_subscriber failed!\n")), 2);

    // Create Publisher
    DDS::Publisher_var publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(publisher.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")), 1);

    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")), 1);

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
                                CORBA::String_var(ts->get_type_name()),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")), 1);

    // Create DataReader
    DDS::DataReaderQos qos;

    if (subscriber->get_default_datareader_qos(qos) != DDS::RETCODE_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader failed!\n")), -1);

    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
                                    qos,
                                    DDS::DataReaderListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datareader failed!\n")), 7);

    FooDataReader_var reader_i = FooDataReader::_narrow(reader);
    if (CORBA::is_nil(reader_i))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: _narrow failed!\n")), 1);

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic.in(),
                                   DATAWRITER_QOS_DEFAULT,
                                   DDS::DataWriterListener::_nil(),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(writer.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")), 1);

    FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
    if (CORBA::is_nil(writer_i))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: _narrow failed!\n")), 1);

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
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: wait failed!\n")), 1);

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: failed to get publication matched status!\n")),
                          2);
      }
    }
    while (matches.current_count < 1);

    ws->detach_condition(cond);

    //
    // FooDataWriter::dispose should cause an instance to be
    // deleted after the last sample in the instance has been
    // taken from the ReceivedDataElementList:
    //
    DDS_TEST test(reader_i);
    if (!test) return 1;

    Foo foo;
    DDS::InstanceHandle_t handle;

    handle = writer_i->register_instance(foo);

    writer_i->write(foo, handle);
    writer_i->dispose(foo, handle);

    ACE_OS::sleep(5); // wait for samples to arrive

    if (!test.take_all_samples())
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: unable to take samples!\n")), 2);
    /// Verify instance has been deleted
    if (test.has_instance(handle))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: instance not removed!\n")), 3);

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in main()");
    return -1;
  }

  return 0;
}
