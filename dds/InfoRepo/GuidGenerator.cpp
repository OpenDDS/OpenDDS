// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "GuidGenerator.h"
#include "ace/Log_Msg.h"

GuidGenerator::GuidGenerator(
  long                      federation,
  long                      participant,
  OpenDDS::DCPS::EntityKind kind
) : kind_( kind),
    federation_( federation),
    participant_( participant),
    lastKey_( 0),
    kindCode_(
      (kind == OpenDDS::DCPS::KIND_WRITER)? OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY:
      (kind == OpenDDS::DCPS::KIND_READER)? OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY:
      (kind == OpenDDS::DCPS::KIND_TOPIC)?  OpenDDS::DCPS::ENTITYKIND_OPENDDS_TOPIC:
      0x0
    )
{
}

GuidGenerator::~GuidGenerator()
{
}

    /// Obtain the next GUID value.
OpenDDS::DCPS::GUID_t
GuidGenerator::next()
{
  // Generate a new key value.
  ++this->lastKey_;

  // Generate a Participant GUID value.
  if( this->kind_ == OpenDDS::DCPS::KIND_PARTICIPANT) {

    // Rudimentary validity checking.
    if( this->lastKey_ == 0) {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: GuidGenerator::next: ")
        ACE_TEXT("Exceeded Maximum number of participant keys!  ")
        ACE_TEXT("Next key will be a duplicate!\n")
      ));
    }

    // Load Vendor Id, federation and participant values.
    OpenDDS::DCPS::GuidConverter converter( this->federation_, this->lastKey_);

    // Extract the GUID.
    OpenDDS::DCPS::GUID_t value = converter;

    // Load the entityKey and entityKind values.
    value.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

    return value;

  // Generate an Entity GUID value.
  } else {

    // Rudimentary validity checking.
    if( (this->lastKey_ & ~OpenDDS::DCPS::GuidConverter::KeyMask) != 0) {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: GuidGenerator::next: ")
        ACE_TEXT("Exceeded Maximum number of entity keys!  ")
        ACE_TEXT("Next key will be a duplicate!\n")
      ));
    }

    OpenDDS::DCPS::GuidConverter converter( this->federation_, this->participant_);

    converter.key()[0] = (this->lastKey_ >> 16) & 0xff;
    converter.key()[1] = (this->lastKey_ >>  8) & 0xff;
    converter.key()[2] =  this->lastKey_        & 0xff;
    converter.kind()   =  this->kindCode_;

    return converter;
  }
}

void
GuidGenerator::last( long key)
{
  if( key > this->lastKey_) {
    this->lastKey_ = key;
  }
}

