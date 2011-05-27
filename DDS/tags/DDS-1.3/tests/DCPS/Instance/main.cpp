/*
 * $Id$
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
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>

#include "FooTypeTypeSupportImpl.h"

#ifdef ACE_AS_STATIC_LIBS                                                   
# include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>                         
#endif
  
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
  take_next_sample()
  {
    Foo foo;
    DDS::SampleInfo si;

    return reader_->take_next_sample(foo, si) == DDS::RETCODE_OK;
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
    TheParticipantFactoryWithArgs(argc, argv);

    // Create Participant
    DDS::DomainParticipant_var participant =
      TheParticipantFactory->create_participant(42,
                                                PARTICIPANT_QOS_DEFAULT,
                                                DDS::DomainParticipantListener::_nil());

    if (CORBA::is_nil(participant.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")), 1);

    // Create Subscriber
    DDS::Subscriber_var subscriber =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil());

    if (CORBA::is_nil(subscriber.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_subscriber failed!\n")), 2);

    // Create Publisher
    DDS::Publisher_var publisher =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil());
   
    if (CORBA::is_nil(publisher.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")), 1);

    OpenDDS::DCPS::TransportIdType transportId(0);

    // Attach Subscriber Transport
    ++transportId;

    OpenDDS::DCPS::TransportConfiguration_rch sub_config =
      TheTransportFactory->get_or_create_configuration(transportId, ACE_TEXT("SimpleTcp"));

    OpenDDS::DCPS::TransportImpl_rch sub_transport =
      TheTransportFactory->create_transport_impl(transportId);

    OpenDDS::DCPS::SubscriberImpl* subscriber_i =
      dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>(subscriber.in());
    
    if (subscriber_i == 0)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: dynamic_cast failed!\n")), 1);

    OpenDDS::DCPS::AttachStatus sub_status =
      subscriber_i->attach_transport(sub_transport.in());
    
    if (sub_status != OpenDDS::DCPS::ATTACH_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: attach_transport failed!\n")), 1);

    // Attach Publisher Transport
    ++transportId;

    OpenDDS::DCPS::TransportConfiguration_rch pub_config =
      TheTransportFactory->get_or_create_configuration(transportId, ACE_TEXT("SimpleTcp"));

    OpenDDS::DCPS::TransportImpl_rch pub_transport =
      TheTransportFactory->create_transport_impl(transportId);

    OpenDDS::DCPS::PublisherImpl* publisher_i =
      dynamic_cast<OpenDDS::DCPS::PublisherImpl*>(publisher.in());
    
    if (publisher_i == 0)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: dynamic_cast failed!\n")), 1);

    OpenDDS::DCPS::AttachStatus pub_status =
      publisher_i->attach_transport(pub_transport.in());
    
    if (pub_status != OpenDDS::DCPS::ATTACH_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: attach_transport failed!\n")), 1);
    
    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")), 1);

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
                                ts->get_type_name(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil());

    if (CORBA::is_nil(topic.in()))
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")), 1);

    // Create DataReader
    DDS::DataReader_var reader =
      subscriber->create_datareader(topic.in(),
                                    DATAREADER_QOS_DEFAULT,
                                    DDS::DataReaderListener::_nil());

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
                                   DDS::DataWriterListener::_nil());
    
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
    cond->set_enabled_statuses(DDS::PUBLICATION_MATCH_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(cond);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITY_SEC, DDS::DURATION_INFINITY_NSEC };

    DDS::ConditionSeq conditions; 
    DDS::PublicationMatchStatus matches = { 0, 0, 0, 0, 0 };
    do
    {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: wait failed!\n")), 1);

      matches = writer->get_publication_match_status();
    }
    while (matches.current_count < 1);

    ws->detach_condition(cond);

    //
    // ASSERTION
    //
    // dispose/unregister should cause an instance to be removed
    // after the last sample in the instance has been removed
    // from the ReceivedDataElementList.
    //
    DDS_TEST test(reader_i);
    if (!test) return 1;
    
    Foo foo;
    DDS::InstanceHandle_t handle;

    handle = writer_i->_cxx_register(foo);

    writer_i->write(foo, handle);
    writer_i->unregister(foo, handle);

    ACE_OS::sleep(10); // wait for samples to arrive

    /// Take sample (this should cause the instance to be removed)
    if (!test.take_next_sample())
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: unable to take next instance!\n")), 2);

    /// Verify instance has been removed
    if (test.has_instance(handle))
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: instance not removed!\n")), 3);

    // Clean-up!    
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
