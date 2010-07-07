
#include "Service_T.h"

template< typename ModelName>
inline
OpenDDS::Model::Service< ModelName>::Service()
{
  this->delegate_.service() = this;

  for( int index = 0; index < ModelName::Elements::Participants::LAST_INDEX; ++index) {
    this->participants_[ index] = 0;
  }

  for( int outter = 0; outter < ModelName::Elements::Participants::LAST_INDEX; ++outter) {
    for( int inner = 0; inner < ModelName::Elements::Types::LAST_INDEX; ++inner) {
      this->types_[ outter][ inner] = false;
    }
  }

  for( int outter = 0; outter < ModelName::Elements::Participants::LAST_INDEX; ++outter) {
    for( int inner = 0; inner < ModelName::Elements::Topics::LAST_INDEX; ++inner) {
      this->topics_[ outter][ inner] = 0;
    }
  }

  for( int index = 0; index < ModelName::Elements::Publishers::LAST_INDEX; ++index) {
    this->publishers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::Elements::Subscribers::LAST_INDEX; ++index) {
    this->subscribers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::Elements::DataWriters::LAST_INDEX; ++index) {
    this->writers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::Elements::DataReaders::LAST_INDEX; ++index) {
    this->readers_[ index] = 0;
  }

  for( int index = 0; index < ModelName::Elements::Transports::LAST_INDEX; ++index) {
    this->transports_[ index] = 0;
  }
}

template< typename ModelName>
inline
OpenDDS::Model::Service< ModelName>::~Service()
{
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::init( int argc, char** argv)
{
  this->delegate_.init( argc, argv);
  this->modelData_.init();
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::fini()
{
}

template< typename ModelName>
inline
DDS::DomainParticipant_var
OpenDDS::Model::Service< ModelName>::participant( typename Participants::Values participant)
{
  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  return DDS::DomainParticipant::_duplicate(this->participants_[ participant]);
}

template< typename ModelName>
inline
DDS::Topic_var
OpenDDS::Model::Service< ModelName>::topic(
  typename Participants::Values participant,
  typename Topics::Values       topic
)
{
  if( !this->topics_[ participant][ topic]) {
    this->createTopic( participant, topic);
  }
  return DDS::Topic::_duplicate(this->topics_[ participant][ topic]);
}

template< typename ModelName>
inline
DDS::Publisher_var
OpenDDS::Model::Service< ModelName>::publisher( typename Publishers::Values publisher)
{
  if( !this->publishers_[ publisher]) {
    this->createPublisher( publisher);
  }
  return DDS::Publisher::_duplicate(this->publishers_[ publisher]);
}

template< typename ModelName>
inline
DDS::Subscriber_var
OpenDDS::Model::Service< ModelName>::subscriber( typename Subscribers::Values subscriber)
{
  if( !this->subscribers_[ subscriber]) {
    this->createSubscriber( subscriber);
  }
  return DDS::Subscriber::_duplicate(this->subscribers_[ subscriber]);
}

template< typename ModelName>
inline
DDS::DataWriter_var
OpenDDS::Model::Service< ModelName>::writer( typename DataWriters::Values writer)
{
  if( !this->writers_[ writer]) {
    this->createPublication( writer);
  }
  return DDS::DataWriter::_duplicate(this->writers_[ writer]);
}

template< typename ModelName>
inline
DDS::DataReader_var
OpenDDS::Model::Service< ModelName>::reader( typename DataReaders::Values reader)
{
  if( !this->readers_[ reader]) {
    this->createSubscription( reader);
  }
  return DDS::DataReader::_duplicate(this->readers_[ reader]);
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createParticipant(
  typename Participants::Values participant
)
{
  this->delegate_.createParticipant(
    this->participants_[ participant],
    this->modelData_.domain( participant),
    this->modelData_.qos( participant),
    this->modelData_.mask( participant)
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createTopic(
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

  this->delegate_.createTopic(
    this->topics_[ participant][ topic],
    this->participants_[ participant],
    this->modelData_.topicName( topic),
    this->modelData_.typeName( type),
    this->modelData_.qos( topic),
    this->modelData_.mask( topic)
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createPublisher(
  typename Publishers::Values publisher
)
{
  typename Participants::Values participant = this->modelData_.participant( publisher);
  typename Transports::Values   transport   = this->modelData_.transport( publisher);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  if( !this->transports_[ transport]) {
    this->createTransport( transport);
  }

  this->delegate_.createPublisher(
    this->publishers_[ publisher],
    this->participants_[ participant],
    this->modelData_.qos( publisher),
    this->modelData_.mask( publisher),
    this->transports_[ transport]
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createSubscriber(
  typename Subscribers::Values subscriber
)
{
  typename Participants::Values participant = this->modelData_.participant( subscriber);
  typename Transports::Values   transport   = this->modelData_.transport( subscriber);

  if( !this->participants_[ participant]) {
    this->createParticipant( participant);
  }
  if( !this->transports_[ transport]) {
    this->createTransport( transport);
  }

  this->delegate_.createSubscriber(
    this->subscribers_[ subscriber],
    this->participants_[ participant],
    this->modelData_.qos( subscriber),
    this->modelData_.mask( subscriber),
    this->transports_[ transport]
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createPublication( typename DataWriters::Values writer)
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

  this->delegate_.createPublication(
    writer,
    this->writers_[ writer],
    this->publishers_[ publisher],
    this->topics_[ participant][ topic],
    this->modelData_.qos( writer),
    this->modelData_.mask( writer)
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createSubscription( typename DataReaders::Values reader)
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

  this->delegate_.createSubscription(
    reader,
    this->readers_[ reader],
    this->subscribers_[ subscriber],
    this->topics_[ participant][ topic],
    this->modelData_.qos( reader),
    this->modelData_.mask( reader)
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::createTransport( typename Transports::Values transport)
{
  this->delegate_.createTransport(
    this->transports_[ transport],
    this->modelData_.transportKey( transport),
    this->modelData_.transportKind( transport),
    this->modelData_.transportConfig( transport)
  );
}

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::copyPublicationQos(
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

template< typename ModelName>
inline
void
OpenDDS::Model::Service< ModelName>::copySubscriptionQos(
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

