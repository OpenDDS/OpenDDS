
#include "Delegate.h"

#include "Exceptions.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"

#include "dds/DCPS/Service_Participant.h"

OpenDDS::Model::Delegate::Delegate()
 : service_( 0)
{
}

void
OpenDDS::Model::Delegate::init( int argc, char** argv)
{
  TheParticipantFactoryWithArgs( argc, argv);
}

void
OpenDDS::Model::Delegate::fini()
{
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}

OpenDDS::Model::CopyQos*&
OpenDDS::Model::Delegate::service()
{
  return this->service_;
}

DDS::DomainParticipant*
OpenDDS::Model::Delegate::createParticipant(
  unsigned long             domain,
  DDS::DomainParticipantQos participantQos,
  DDS::StatusMask           mask
)
{
  DDS::DomainParticipant* participant
    = TheParticipantFactory->create_participant(
        domain,
        participantQos,
        DDS::DomainParticipantListener::_nil(),
        mask
      );
  if( !participant) {
    throw NoParticipantException();
  }
  return participant;
}

DDS::Topic*
OpenDDS::Model::Delegate::createTopic(
  DDS::DomainParticipant* participant,
  const std::string&      topicName,
  const std::string&      typeName,
  DDS::TopicQos           topicQos,
  DDS::StatusMask         mask
)
{
  DDS::Topic* topic
    = participant->create_topic(
        topicName.c_str(),
        typeName.c_str(),
        topicQos,
        DDS::TopicListener::_nil(),
        mask
      );
  if( !topic) {
    throw NoTopicException();
  }
  return topic;
}

DDS::Publisher*
OpenDDS::Model::Delegate::createPublisher(
  DDS::DomainParticipant*       participant,
  DDS::PublisherQos             publisherQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportImpl* transport
)
{
  DDS::Publisher* publisher
    = participant->create_publisher(
        publisherQos,
        DDS::PublisherListener::_nil(),
        mask
      );
  if( !publisher) {
    throw NoPublisherException();
  }

  if( OpenDDS::DCPS::ATTACH_OK != transport->attach( publisher)) {
    throw BadAttachException();
  }

  return publisher;
}

DDS::Subscriber*
OpenDDS::Model::Delegate::createSubscriber(
  DDS::DomainParticipant*       participant,
  DDS::SubscriberQos            subscriberQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportImpl* transport
)
{
  DDS::Subscriber* subscriber
    = participant->create_subscriber(
        subscriberQos,
        DDS::SubscriberListener::_nil(),
        mask
      );
  if( !subscriber) {
    throw NoSubscriberException();
  }

  if( OpenDDS::DCPS::ATTACH_OK != transport->attach( subscriber)) {
    throw BadAttachException();
  }

  return subscriber;
}

DDS::DataWriter*
OpenDDS::Model::Delegate::createPublication(
  unsigned int       which,
  DDS::Publisher*    publisher,
  DDS::Topic*        topic,
  DDS::DataWriterQos writerQos,
  DDS::StatusMask    mask
)
{
  if( !this->service_) {
    throw NoServiceException();
  }

  DDS::TopicQos topicQos = TheServiceParticipant->initial_TopicQos();
  topic->get_qos( topicQos);
  publisher->get_default_datawriter_qos( writerQos);
  publisher->copy_from_topic_qos( writerQos, topicQos);
  this->service_->copyPublicationQos( which, writerQos);

  DDS::DataWriter* writer = publisher->create_datawriter(
                              topic,
                              writerQos,
                              DDS::DataWriterListener::_nil(),
                              mask
                            );
  return writer;
}

DDS::DataReader*
OpenDDS::Model::Delegate::createSubscription(
  unsigned int       which,
  DDS::Subscriber*   subscriber,
  DDS::Topic*        topic,
  DDS::DataReaderQos readerQos,
  DDS::StatusMask    mask
)
{
  if( !this->service_) {
    throw NoServiceException();
  }

  DDS::TopicQos topicQos = TheServiceParticipant->initial_TopicQos();
  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  this->service_->copySubscriptionQos( which, readerQos);

  DDS::DataReader* reader = subscriber->create_datareader(
                              topic,
                              readerQos,
                              DDS::DataReaderListener::_nil(),
                              mask
                            );
  return reader;
}

OpenDDS::DCPS::TransportImpl*
OpenDDS::Model::Delegate::createTransport(
  unsigned long                          key,
  const char*                            kind,
  OpenDDS::DCPS::TransportConfiguration* config
)
{
  OpenDDS::DCPS::TransportImpl_rch result
    = TheTransportFactory->create_transport_impl(
        key,
        kind,
        OpenDDS::DCPS::DONT_AUTO_CONFIG
      );

  OpenDDS::DCPS::TransportImpl* transport = result._retn();
  if( !transport) {
    return 0;
  }

  if( 0 != transport->configure( config)) {
    delete transport; /// @TODO: Check that this is correct.
    return 0;
  }

  return transport;
}

