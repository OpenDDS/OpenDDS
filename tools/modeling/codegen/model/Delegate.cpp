
#include "Delegate.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/multicast/MulticastInst.h"

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
  DDS::StatusMask           mask,
  const std::string&        transportConfig
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
  // If the modeler specified a transport config, bind to it
  if (!transportConfig.empty()) {
    try {
      TheTransportRegistry->bind_config(transportConfig, participant);
    } catch (OpenDDS::DCPS::Transport::Exception&) {
      pfact->delete_participant(participant);
      return 0;
    }
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
  return topic;
}

DDS::Publisher*
OpenDDS::Model::Delegate::createPublisher(
  DDS::DomainParticipant*       participant,
  DDS::PublisherQos             publisherQos,
  DDS::StatusMask               mask,
  const std::string&            transportConfig
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

  // If the modeler specified a transport config, bind to it
  if (!transportConfig.empty()) {
    try {
      TheTransportRegistry->bind_config(transportConfig, publisher);
    } catch (OpenDDS::DCPS::Transport::Exception&) {
      participant->delete_publisher(publisher);
      return 0;
    }
  }

  return publisher;
}

DDS::Subscriber*
OpenDDS::Model::Delegate::createSubscriber(
  DDS::DomainParticipant*       participant,
  DDS::SubscriberQos            subscriberQos,
  DDS::StatusMask               mask,
  const std::string&            transportConfig
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

  // If the modeler specified a transport config, bind to it
  if (!transportConfig.empty()) {
    try {
      TheTransportRegistry->bind_config(transportConfig, subscriber);
    } catch (OpenDDS::DCPS::Transport::Exception&) {
      participant->delete_subscriber(subscriber);
      return 0;
    }
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
  const std::string& transportConfig,
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
           mask,
           transportConfig
         );
}

DDS::DataWriter*
OpenDDS::Model::Delegate::createWriter(
  DDS::Publisher*    publisher,
  DDS::Topic*        topic,
  DDS::DataWriterQos writerQos,
  DDS::StatusMask    mask,
  const std::string& transportConfig
)
{
  // If we specify a transport and have autoenable modeled,
  // temporarily turn off autoenabling
  bool overridden = false;
  bool transportSpecified = !transportConfig.empty();

  // If transport config was specified
  if (transportSpecified) {
    overridden = override_autoenabled_qos(publisher);
  }

  DDS::DataWriter_ptr dataWriter = publisher->create_datawriter(
           topic,
           writerQos,
           DDS::DataWriterListener::_nil(),
           mask
         );

  if (transportSpecified) {
    // Restore autoenabling if necessary
    if (overridden) {
      restore_autoenabled_qos(publisher);
    }
    // bind to specified transport
    TheTransportRegistry->bind_config(transportConfig, dataWriter);
  }

  return dataWriter;
}

DDS::DataReader*
OpenDDS::Model::Delegate::createSubscription(
  unsigned int           which,
  DDS::Subscriber*       subscriber,
  DDS::TopicDescription* topic,
  DDS::DataReaderQos     readerQos,
  DDS::StatusMask        mask,
  const std::string&     transportConfig,
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
           mask,
           transportConfig
         );
}

DDS::DataReader*
OpenDDS::Model::Delegate::createReader(
  DDS::Subscriber*       subscriber,
  DDS::TopicDescription* topic,
  DDS::DataReaderQos     readerQos,
  DDS::StatusMask        mask,
  const std::string&     transportConfig
)
{
  // If we specify a transport and have autoenable modeled,
  // temporarily turn off autoenabling
  bool overridden = false;
  bool transportSpecified = !transportConfig.empty();

  // If transport config was specified
  if (transportSpecified) {
    overridden = override_autoenabled_qos(subscriber);
  }

  DDS::DataReader_ptr dataReader = subscriber->create_datareader(
           topic,
           readerQos,
           DDS::DataReaderListener::_nil(),
           mask
         );

  if (transportSpecified) {
    // Restore autoenabling if necessary
    if (overridden) {
      restore_autoenabled_qos(subscriber);
    }
    // bind to specified transport
    TheTransportRegistry->bind_config(transportConfig, dataReader);
  }
  return dataReader;
}

bool
OpenDDS::Model::Delegate::override_autoenabled_qos(
  DDS::Publisher* publisher
)
{
  DDS::PublisherQos pub_qos;
  publisher->get_qos(pub_qos);

  // Save value
  bool should_restore = pub_qos.entity_factory.autoenable_created_entities;

  // Disable
  pub_qos.entity_factory.autoenable_created_entities = false;

  return should_restore;
}

bool
OpenDDS::Model::Delegate::override_autoenabled_qos(
  DDS::Subscriber* subscriber
)
{
  DDS::SubscriberQos sub_qos;
  subscriber->get_qos(sub_qos);

  // Save value
  bool should_restore = sub_qos.entity_factory.autoenable_created_entities;

  // Disable
  sub_qos.entity_factory.autoenable_created_entities = false;

  return should_restore;
}

void
OpenDDS::Model::Delegate::restore_autoenabled_qos(
  DDS::Publisher* publisher
)
{
  DDS::PublisherQos pub_qos;
  publisher->get_qos(pub_qos);
  pub_qos.entity_factory.autoenable_created_entities = true;
  publisher->set_qos(pub_qos);
}

void
OpenDDS::Model::Delegate::restore_autoenabled_qos(
  DDS::Subscriber* subscriber
)
{
  DDS::SubscriberQos sub_qos;
  subscriber->get_qos(sub_qos);
  sub_qos.entity_factory.autoenable_created_entities = true;
  subscriber->set_qos(sub_qos);
}
