
#include "Service_T.h"
#include "Application.h"
#include <ace/Log_Msg.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template< typename ModelName, class InstanceTraits>
inline
OpenDDS::Model::Service<ModelName, InstanceTraits>::Service(
  const Application& application,
  int& argc,
  ACE_TCHAR* argv[])
  : Entities(argc, argv)
  , application_(application)
  , modelData_()
  , participants_(Participants::LAST_INDEX)
  , types_(Participants::LAST_INDEX)
  , topics_(Participants::LAST_INDEX)
  , publishers_(Publishers::LAST_INDEX)
  , subscribers_(Subscribers::LAST_INDEX)
  , writers_(DataWriters::LAST_INDEX)
  , readers_(DataReaders::LAST_INDEX)
{
  this->delegate_.service() = this;

  for (int outer = 0; outer < Participants::LAST_INDEX; ++outer) {
    this->types_[outer].resize(Types::LAST_INDEX);
    this->topics_[outer].resize(Topics::LAST_INDEX);
  }
}

template< typename ModelName, class InstanceTraits>
inline
OpenDDS::Model::Service< ModelName, InstanceTraits>::~Service()
{
  DDS::DomainParticipantFactory_var pfact = TheParticipantFactory;
  for( int index = 0; index < Participants::LAST_INDEX; ++index) {
    if( this->participants_[ index]) {
      this->participants_[ index]->delete_contained_entities();
      pfact->delete_participant( this->participants_[ index]);
    }
  }
}

template< typename ModelName, class InstanceTraits>
inline
DDS::DomainParticipant_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::participant( typename Participants::Values participant)
{
  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  return DDS::DomainParticipant::_duplicate(this->participants_[ participant]);
}

template< typename ModelName, class InstanceTraits>
inline
DDS::TopicDescription_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::topic(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  if(!this->topics_[ participant][ topic]) {
    this->createTopicDescription(participant, topic);
  }
  return this->topics_[ participant][ topic];
}

template< typename ModelName, class InstanceTraits>
inline
DDS::Publisher_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::publisher( typename Publishers::Values publisher)
{
  if( !this->publishers_[ publisher]) {
    this->createPublisher( publisher);
  }
  return DDS::Publisher::_duplicate(this->publishers_[ publisher]);
}

template< typename ModelName, class InstanceTraits>
inline
DDS::Subscriber_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::subscriber( typename Subscribers::Values subscriber)
{
  if( !this->subscribers_[ subscriber]) {
    this->createSubscriber( subscriber);
  }
  return DDS::Subscriber::_duplicate(this->subscribers_[ subscriber]);
}

template< typename ModelName, class InstanceTraits>
inline
DDS::DataWriter_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::writer( typename DataWriters::Values writer)
{
  if( !this->writers_[ writer]) {
    this->createPublication( writer);
  }
  return DDS::DataWriter::_duplicate(this->writers_[ writer]);
}

template< typename ModelName, class InstanceTraits>
inline
DDS::DataReader_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::reader( typename DataReaders::Values reader)
{
  if( !this->readers_[ reader]) {
    this->createSubscription( reader);
  }
  return DDS::DataReader::_duplicate(this->readers_[ reader]);
}

template< typename ModelName, class InstanceTraits>
inline
DDS::DomainParticipant*
OpenDDS::Model::Service< ModelName, InstanceTraits>::createParticipant(
  typename Participants::Values participant
)
{
  return this->participants_[participant] = this->delegate_.createParticipant(
           this->modelData_.domain( participant),
           this->modelData_.qos( participant),
           this->modelData_.mask( participant),
           this->configName(this->modelData_.transportConfigName(participant))
         );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createTopicDescription(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  typename ContentFilteredTopics::Values cfTopic =
               this->modelData_.contentFilteredTopic(topic);
  typename MultiTopics::Values multiTopic =
               this->modelData_.multiTopic(topic);
  // If this is a content-filtered topic
  if (cfTopic != ContentFilteredTopics::LAST_INDEX) {
    createContentFilteredTopic(participant, topic, cfTopic);
  // Else if this is a multitopic
  } else if (multiTopic != MultiTopics::LAST_INDEX) {
    createMultiTopic(participant, topic, multiTopic);
  // Else this is a standard topic
  } else {
    createTopic(participant, topic);
  }
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createTopic(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  typename Types::Values type = this->modelData_.type(topic);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  if( !this->types_[ participant][ type]) {
    this->modelData_.registerType( type, this->participants_[ participant]);
    this->types_[ participant][ type] = true;
  }

  this->topics_[participant][topic] = this->delegate_.createTopic(
    this->participants_[participant],
    this->modelData_.topicName(topic),
    this->modelData_.typeName(type),
    this->modelData_.qos(topic),
    this->modelData_.mask(topic)
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createContentFilteredTopic(
  typename Participants::Values          participant,
  typename Topics::Values                topic,
  typename ContentFilteredTopics::Values cfTopic
)
{
  const char* topicName = this->modelData_.topicName(topic);
  typename Topics::Values target_topic = this->modelData_.relatedTopic(cfTopic);
  DDS::Topic_var related_topic =
        dynamic_cast<DDS::Topic*>(this->topic(participant, target_topic).ptr());
  const char* filter_expression = this->modelData_.filterExpression(cfTopic);
  DDS::DomainParticipant_var domain_participant = this->participant(participant);
  // TODO: Should this be moved to Delegate?
  this->topics_[participant][topic] =
        domain_participant->create_contentfilteredtopic(topicName,
                                                        related_topic,
                                                        filter_expression,
                                                        DDS::StringSeq());
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createMultiTopic(
  typename Participants::Values participant,
  typename Topics::Values       topic,
  typename MultiTopics::Values  multiTopic
)
{
  const char* topicName = this->modelData_.topicName(topic);
  typename Types::Values type = this->modelData_.type(topic);
  DDS::DomainParticipant_var domain_participant = this->participant(participant);
  const char* topicExpression = this->modelData_.topicExpression(multiTopic);

  if(!this->types_[participant][type]) {
    this->modelData_.registerType(type, this->participants_[ participant]);
    this->types_[participant][type] = true;
  }
  // TODO: Should this be moved to Delegate?
  this->topics_[participant][topic] =
        domain_participant->create_multitopic(topicName,
                                              this->modelData_.typeName(type),
                                              topicExpression,
                                              DDS::StringSeq());
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createPublisher(
  typename Publishers::Values publisher
)
{
  typename Participants::Values participant = this->modelData_.participant( publisher);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }

  this->publishers_[ publisher] = this->delegate_.createPublisher(
    this->participants_[ participant],
    this->modelData_.qos( publisher),
    this->modelData_.mask( publisher),
    this->configName(this->modelData_.transportConfigName(publisher))
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createSubscriber(
  typename Subscribers::Values subscriber
)
{
  typename Participants::Values participant = this->modelData_.participant( subscriber);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }

  this->subscribers_[ subscriber] = this->delegate_.createSubscriber(
    this->participants_[ participant],
    this->modelData_.qos( subscriber),
    this->modelData_.mask( subscriber),
    this->configName(this->modelData_.transportConfigName(subscriber))
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createPublication( typename DataWriters::Values writer)
{
  typename Publishers::Values   publisher   = this->modelData_.publisher( writer);
  typename Participants::Values participant = this->modelData_.participant( publisher);
  typename Topics::Values       topic       = this->modelData_.topic( writer);

  if( !this->publishers_[ publisher]) {
    this->createPublisher( publisher);
  }
  if( !this->topics_[ participant][ topic]) {
    this->createTopic( participant, topic);
  }

  // TODO: Figure out how to get this as a Topic*
  this->writers_[writer] = this->delegate_.createPublication(
    writer,
    this->publishers_[publisher],
    dynamic_cast<DDS::Topic*>(this->topics_[participant][topic].ptr()),
    this->modelData_.qos(writer),
    this->modelData_.mask(writer),
    this->configName(this->modelData_.transportConfigName(writer)),
    this->modelData_.copyTopicQos(writer)
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createSubscription( typename DataReaders::Values reader)
{
  typename Subscribers::Values  subscriber  = this->modelData_.subscriber( reader);
  typename Participants::Values participant = this->modelData_.participant( subscriber);
  typename Topics::Values       topic       = this->modelData_.topic( reader);

  if( !this->subscribers_[ subscriber]) {
    this->createSubscriber( subscriber);
  }
  if( !this->topics_[ participant][ topic]) {
    this->createTopicDescription(participant, topic);
  }

  this->readers_[reader] = this->delegate_.createSubscription(
    reader,
    this->subscribers_[subscriber],
    this->topics_[participant][topic],
    this->modelData_.qos(reader),
    this->modelData_.mask(reader),
    this->configName(this->modelData_.transportConfigName(reader)),
    this->modelData_.copyTopicQos(reader)
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::loadTransportLibraryIfNeeded(
  typename Transport::Type::Values transport_type
)
{
  const ACE_TCHAR *svcName;
  const ACE_TCHAR *svcConfDir;

  if (transport_type == Transport::Type::tcp)
  {
    svcName    = OpenDDS::Model::Transport::Tcp::svcName;
    svcConfDir = OpenDDS::Model::Transport::Tcp::svcConfDir;
  } else if (transport_type == Transport::Type::udp) {
    svcName    = OpenDDS::Model::Transport::Udp::svcName;
    svcConfDir = OpenDDS::Model::Transport::Udp::svcConfDir;
  } else if (transport_type == Transport::Type::multicast) {
    svcName    = OpenDDS::Model::Transport::Multicast::svcName;
    svcConfDir = OpenDDS::Model::Transport::Multicast::svcConfDir;
  } else {
    throw std::runtime_error("unknown transport type");
  }

  ACE_Service_Gestalt *asg = ACE_Service_Config::current();

  if (asg->find(svcName) == -1 /*not found*/) {
    int errors = ACE_Service_Config::process_directive(svcConfDir);
    if (errors) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("Had %d errors processing directive for %s\n"),
                 errors, svcName));
    }
  }

}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::copyPublicationQos(
  unsigned int        which,
  DDS::DataWriterQos& writerQos
)
{
  typename DataWriters::Values writer = static_cast< typename DataWriters::Values>( which);
  if( writer < 0 || writer >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  this->modelData_.copyPublicationQos( writer, writerQos);
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::copySubscriptionQos(
  unsigned int        which,
  DDS::DataReaderQos& readerQos
)
{
  typename DataReaders::Values reader = static_cast< typename DataReaders::Values>( which);
  if( reader < 0 || reader >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  this->modelData_.copySubscriptionQos( reader, readerQos);
}

template< typename ModelName, class InstanceTraits>
inline
const OPENDDS_STRING
OpenDDS::Model::Service< ModelName, InstanceTraits>::transportConfigName(
  typename Participants::Values which
) {
  return this->modelData_.transportConfigName(which);
}

template< typename ModelName, class InstanceTraits>
inline
const OPENDDS_STRING
OpenDDS::Model::Service< ModelName, InstanceTraits>::transportConfigName(
  typename Publishers::Values which
) {
  return this->modelData_.transportConfigName(which);
}

template< typename ModelName, class InstanceTraits>
inline
const OPENDDS_STRING
OpenDDS::Model::Service< ModelName, InstanceTraits>::transportConfigName(
  typename Subscribers::Values which
) {
  return this->modelData_.transportConfigName(which);
}

template< typename ModelName, class InstanceTraits>
inline
const OPENDDS_STRING
OpenDDS::Model::Service< ModelName, InstanceTraits>::transportConfigName(
  typename DataWriters::Values which
) {
  return this->modelData_.transportConfigName(which);
}

template< typename ModelName, class InstanceTraits>
inline
const OPENDDS_STRING
OpenDDS::Model::Service< ModelName, InstanceTraits>::transportConfigName(
  typename DataReaders::Values which
) {
  return this->modelData_.transportConfigName(which);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
