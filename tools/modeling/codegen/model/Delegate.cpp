
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

OpenDDS::Model::CopyQos*&
OpenDDS::Model::Delegate::service()
{
  return this->service_;
}

void
OpenDDS::Model::Delegate::createParticipant(
  DDS::DomainParticipant*&  participant,
  unsigned long             domain,
  DDS::DomainParticipantQos participantQos,
  DDS::StatusMask           mask
)
{
  participant = TheParticipantFactory->create_participant(
                  domain,
                  participantQos,
                  DDS::DomainParticipantListener::_nil(),
                  mask
                );
  if( !participant) {
    throw NoParticipantException();
  }
}

void
OpenDDS::Model::Delegate::createTopic(
  DDS::Topic*&            topic,
  DDS::DomainParticipant* participant,
  const char*             topicName,
  const char*             typeName,
  DDS::TopicQos           topicQos,
  DDS::StatusMask         mask
)
{
  topic = participant->create_topic(
            topicName,
            typeName,
            topicQos,
            DDS::TopicListener::_nil(),
            mask
          );
  if( !topic) {
    throw NoTopicException();
  }
}

void
OpenDDS::Model::Delegate::createPublisher(
  DDS::Publisher*&              publisher,
  DDS::DomainParticipant*       participant,
  DDS::PublisherQos             publisherQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportImpl* transport
)
{
  publisher = participant->create_publisher(
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
}

void
OpenDDS::Model::Delegate::createSubscriber(
  DDS::Subscriber*&             subscriber,
  DDS::DomainParticipant*       participant,
  DDS::SubscriberQos            subscriberQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportImpl* transport
)
{
  subscriber = participant->create_subscriber(
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
}

void
OpenDDS::Model::Delegate::createPublication(
  unsigned int       which,
  DDS::DataWriter*&  writer,
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

  writer = publisher->create_datawriter(
        topic,
        writerQos,
        DDS::DataWriterListener::_nil(),
        mask
      );
  if( !writer) {
    throw NoWriterException();
  }
}

void
OpenDDS::Model::Delegate::createSubscription(
  unsigned int       which,
  DDS::DataReader*&  reader,
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

  reader = subscriber->create_datareader(
        topic,
        readerQos,
        DDS::DataReaderListener::_nil(),
        mask
      );
  if( !reader) {
    throw NoReaderException();
  }
}

void
OpenDDS::Model::Delegate::createTransport(
  OpenDDS::DCPS::TransportImpl*&         transport,
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

  transport = result._retn();
  if( !transport) {
    throw NoTransportException();
  }

  if( 0 != transport->configure( config)) {
    throw BadConfigureException();
  }
}

