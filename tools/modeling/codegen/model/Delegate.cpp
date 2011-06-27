
#include "Delegate.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/tcp/TcpConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"

#include "dds/DCPS/Service_Participant.h"

OpenDDS::Model::Delegate::Delegate()
  : service_(0)
{
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
  DDS::DomainParticipantFactory_var pfact = TheParticipantFactory;
  DDS::DomainParticipant* participant
    = pfact->create_participant(
        domain,
        participantQos,
        DDS::DomainParticipantListener::_nil(),
        mask
      );
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
  return topic;
}

DDS::Publisher*
OpenDDS::Model::Delegate::createPublisher(
  DDS::DomainParticipant*       participant,
  DDS::PublisherQos             publisherQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportIdType transport_id
)
{
  DDS::Publisher* publisher
    = participant->create_publisher(
        publisherQos,
        DDS::PublisherListener::_nil(),
        mask
      );
  if( !publisher) {
    return 0;
  }

  OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->obtain(transport_id);

  if( OpenDDS::DCPS::ATTACH_OK != transport->attach( publisher)) {
    participant->delete_publisher( publisher);
    return 0;
  }

  return publisher;
}

DDS::Subscriber*
OpenDDS::Model::Delegate::createSubscriber(
  DDS::DomainParticipant*       participant,
  DDS::SubscriberQos            subscriberQos,
  DDS::StatusMask               mask,
  OpenDDS::DCPS::TransportIdType transport_id
)
{
  DDS::Subscriber* subscriber
    = participant->create_subscriber(
        subscriberQos,
        DDS::SubscriberListener::_nil(),
        mask
      );
  if( !subscriber) {
    return 0;
  }

  OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->obtain(transport_id);

  if( OpenDDS::DCPS::ATTACH_OK != transport->attach( subscriber)) {
    participant->delete_subscriber( subscriber);
    return 0;
  }

  return subscriber;
}

DDS::DataWriter*
OpenDDS::Model::Delegate::createPublication(
  unsigned int       which,
  DDS::Publisher*    publisher,
  DDS::Topic*        topic,
  DDS::DataWriterQos writerQos,
  DDS::StatusMask    mask,
  bool               copyQosFromTopic
)
{
  if( !this->service_) {
    return 0;
  }

  DDS::TopicQos topicQos = TheServiceParticipant->initial_TopicQos();
  topic->get_qos( topicQos);
  publisher->get_default_datawriter_qos( writerQos);
  if (copyQosFromTopic) {
    publisher->copy_from_topic_qos(writerQos, topicQos);
  }
  this->service_->copyPublicationQos( which, writerQos);

  return this->createWriter(
           publisher,
           topic,
           writerQos,
           mask
         );
}

DDS::DataWriter*
OpenDDS::Model::Delegate::createWriter(
  DDS::Publisher*    publisher,
  DDS::Topic*        topic,
  DDS::DataWriterQos writerQos,
  DDS::StatusMask    mask
)
{
  return publisher->create_datawriter(
           topic,
           writerQos,
           DDS::DataWriterListener::_nil(),
           mask
         );
}

DDS::DataReader*
OpenDDS::Model::Delegate::createSubscription(
  unsigned int           which,
  DDS::Subscriber*       subscriber,
  DDS::TopicDescription* topic,
  DDS::DataReaderQos     readerQos,
  DDS::StatusMask        mask,
  bool                   copyQosFromTopic
)
{
  if( !this->service_) {
    return 0;
  }

  subscriber->get_default_datareader_qos( readerQos);
  if (copyQosFromTopic) {
    // Per the DDS Spec, copy from related topic for CF topic,
    // Error if copy from mulitopic
    DDS::TopicQos topicQos = TheServiceParticipant->initial_TopicQos();
    DDS::Topic* qosTopic;
    if ((qosTopic = dynamic_cast<DDS::Topic*>(topic)) != NULL) {
      qosTopic->get_qos(topicQos);
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    } else {
      DDS::ContentFilteredTopic* qosCfTopic =
                    dynamic_cast<DDS::ContentFilteredTopic*>(topic);
      if (qosCfTopic != NULL) {
        qosCfTopic->get_related_topic()->get_qos(topicQos);
      }
#endif
    }
    subscriber->copy_from_topic_qos(readerQos, topicQos);
  }
  this->service_->copySubscriptionQos( which, readerQos);

  return this->createReader(
           subscriber,
           topic,
           readerQos,
           mask
         );
}

DDS::DataReader*
OpenDDS::Model::Delegate::createReader(
  DDS::Subscriber*       subscriber,
  DDS::TopicDescription* topic,
  DDS::DataReaderQos     readerQos,
  DDS::StatusMask        mask
)
{
  return subscriber->create_datareader(
           topic,
           readerQos,
           DDS::DataReaderListener::_nil(),
           mask
         );
}

