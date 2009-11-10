/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorData.h"

#include "dds/DCPS/Service_Participant.h"

namespace Monitor {

MonitorData::MonitorData( const std::string& /* ior */)
{
#if 0
  // Grab a participant for subscribing.
  this->participant_ = TheParticipantFactory->create_participant(
    this->config_.nodeId(),
    PARTICIPANT_QOS_DEFAULT,
    ::DDS::DomainParticipantListener::_nil()
  );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: create_participant failed\n")));
    throw;
  }

  // Grab the built in topic subscriber.
  this->builtinSubscriber_ = this->participant_->get_builtin_subscriber();

  //////////////////////////////////////
  //
  // Publication Builtin Topic
  //

  // Extract the publication builtin topic reader.
  ::DDS::DataReader_var builtinReader
    = this->builtinSubscriber_->lookup_datareader(
        OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC
      );
  this->publicationReader_
    = ::DDS::PublicationBuiltinTopicDataDataReader::_narrow( builtinReader.in());
  if( CORBA::is_nil( this->publicationReader_.in()))
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: failed to get BUILT_IN_PUBLICATION_TOPIC\n")
    ));
    throw;
  }

  // Create the BuiltinTopic listener for publications.
  PublicationBitListener* bitListenerServant
    = new PublicationBitListener( this->config_.nodeId(), *this);
  if( 0 == bitListenerServant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: failed to get builtin topic\n")
    ));
    throw;
  }

  // Add the builtin topic listener.
  this->publicationReader_->set_listener(
    bitListenerServant,
    OpenDDS::DCPS::DEFAULT_STATUS_KIND_MASK
  );

  // PublicationManager processing for existing publications.
  this->processCurrentSamples(
    bitListenerServant,
    this->publicationReader_
  );

#endif
}

MonitorData::~MonitorData()
{
}

void
MonitorData::processCurrentSamples(
  ::DDS::DataReaderListener_var                    /* listener */,
  ::DDS::PublicationBuiltinTopicDataDataReader_var /* reader */
)
{
}

} // End of namespace Monitor

