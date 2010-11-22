
#include "Service_T.h"

template< typename ModelName, class InstanceTraits>
inline
OpenDDS::Model::Service<ModelName, InstanceTraits>::Service(int& argc, char** argv)
  : Entities(argc, argv)
{
  this->delegate_.service() = this;

  for( int index = 0; index < ModelName::Participants::LAST_INDEX; ++index) {
    this->participants_[ index] = 0;
  }

  for( int outter = 0; outter < ModelName::Participants::LAST_INDEX; ++outter) {
    for( int inner = 0; inner < ModelName::Types::LAST_INDEX; ++inner) {
      this->types_[ outter][ inner] = false;
    }
  }

  for( int outter = 0; outter < ModelName::Participants::LAST_INDEX; ++outter) {
    for( int inner = 0; inner < ModelName::Topics::LAST_INDEX; ++inner) {
      this->topics_[ outter][ inner] = 0;
    }
  }

  for( int index = 0; index < ModelName::Publishers::LAST_INDEX; ++index) {
    this->publishers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::Subscribers::LAST_INDEX; ++index) {
    this->subscribers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::DataWriters::LAST_INDEX; ++index) {
    this->writers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::DataReaders::LAST_INDEX; ++index) {
    this->readers_[ index] = 0;
  }
}

template< typename ModelName, class InstanceTraits>
inline
OpenDDS::Model::Service< ModelName, InstanceTraits>::~Service()
{
  for( int index = 0; index < Participants::LAST_INDEX; ++index) {
    if( this->participants_[ index]) {
      this->participants_[ index]->delete_contained_entities();
      TheParticipantFactory->delete_participant( this->participants_[ index]);
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
DDS::Topic_var
OpenDDS::Model::Service< ModelName, InstanceTraits>::topic(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  if( !this->topics_[ participant][ topic]) {
    this->createTopic( participant, topic);
  }
  return DDS::Topic::_duplicate(this->topics_[ participant][ topic]);
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
           this->modelData_.mask( participant)
         );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createTopic(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  typename Types::Values type = this->modelData_.type( topic);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  if( !this->types_[ participant][ type]) {
    this->modelData_.registerType( type, this->participants_[ participant]);
    this->types_[ participant][ type] = true;
  }

  this->topics_[ participant][ topic] = this->delegate_.createTopic(
    this->participants_[ participant],
    this->modelData_.topicName( topic),
    this->modelData_.typeName( type),
    this->modelData_.qos( topic),
    this->modelData_.mask( topic)
  );
}

template< typename ModelName, class InstanceTraits>
inline
void
OpenDDS::Model::Service< ModelName, InstanceTraits>::createPublisher(
  typename Publishers::Values publisher
)
{
  typename Participants::Values participant = this->modelData_.participant( publisher);
  OpenDDS::DCPS::TransportIdType transport = this->modelData_.transport( publisher);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }

  this->transport_config(transport);

  this->publishers_[ publisher] = this->delegate_.createPublisher(
    this->participants_[ participant],
    this->modelData_.qos( publisher),
    this->modelData_.mask( publisher),
    transport
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
  OpenDDS::DCPS::TransportIdType transport = this->modelData_.transport( subscriber);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }

  transport_config(transport);

  this->subscribers_[ subscriber] = this->delegate_.createSubscriber(
    this->participants_[ participant],
    this->modelData_.qos( subscriber),
    this->modelData_.mask( subscriber),
    transport
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

  this->writers_[ writer] = this->delegate_.createPublication(
    writer,
    this->publishers_[ publisher],
    this->topics_[ participant][ topic],
    this->modelData_.qos( writer),
    this->modelData_.mask( writer)
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
    this->createTopic( participant, topic);
  }

  this->readers_[ reader] = this->delegate_.createSubscription(
    reader,
    this->subscribers_[ subscriber],
    this->topics_[ participant][ topic],
    this->modelData_.qos( reader),
    this->modelData_.mask( reader)
  );
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
