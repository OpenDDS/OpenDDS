// -*- C++ -*-
//
// $Id$

ACE_INLINE
void
OpenDDS::Model::Entities::init( int argc, ACE_TCHAR** argv)
{
  // Start the DDS service.
  this->delegate_.init( argc, argv);

  // Initialize from the command line and any file(s).
  this->config_.init( argc, argv);
}

ACE_INLINE
void
OpenDDS::Model::Entities::fini()
{
  this->delegate_.fini();
}

ACE_INLINE
void
OpenDDS::Model::Entities::add( const std::string& name, DDS::DomainParticipant_ptr participant)
{
  this->participantByString_[ name] = participant;
}

ACE_INLINE
void
OpenDDS::Model::Entities::add(
  const std::string& name,
  const std::string& participant,
  DDS::Topic_ptr     topic
)
{
  this->topicByParticipant_[ participant][ name] = topic;
}

ACE_INLINE
void
OpenDDS::Model::Entities::add( const std::string& name, DDS::Publisher_ptr  publisher)
{
  this->publisherByString_[ name] = publisher;
}

ACE_INLINE
void
OpenDDS::Model::Entities::add( const std::string& name, DDS::Subscriber_ptr subscriber)
{
  this->subscriberByString_[ name] = subscriber;
}

ACE_INLINE
void
OpenDDS::Model::Entities::add( const std::string& name, DDS::DataWriter_ptr writer)
{
  this->writerByString_[ name] = writer;
}

ACE_INLINE
void
OpenDDS::Model::Entities::add( const std::string& name, DDS::DataReader_ptr reader)
{
  this->readerByString_[ name] = reader;
}

template< typename TypeSupport>
ACE_INLINE
void
OpenDDS::Model::Entities::add(
  const std::string& name,
  const std::string& participant
)
{
  // Create a Type Support object and grab the type name from it.
  TypeSupport* support = new TypeSupport;
  this->typeNameByString_[ name] = support->get_type_name();

  // Save the TypeSupport object to register it later.
  this->typeSupport_[ participant].push( support);
}

