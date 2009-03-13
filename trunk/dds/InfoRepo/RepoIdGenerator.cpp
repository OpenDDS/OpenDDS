/*
 * $Id$
 */

#include "ace/Log_Msg.h"

#include "DcpsInfo_pch.h"
#include "RepoIdGenerator.h"

const unsigned RepoIdGenerator::KeyBits = 24;

const unsigned RepoIdGenerator::KeyMask = (1 << KeyBits) - 1;

RepoIdGenerator::RepoIdGenerator(
  long                      federation,
  long                      participant,
  OpenDDS::DCPS::EntityKind kind
) : kind_( kind),
    federation_( federation),
    participant_( participant),
    lastKey_( 0),
    kindCode_(OpenDDS::DCPS::get_entity_kind(kind))
{
}

RepoIdGenerator::~RepoIdGenerator()
{
}

OpenDDS::DCPS::RepoId
RepoIdGenerator::next()
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
        ACE_TEXT("(%P|%t) ERROR: RepoIdGenerator::next: ")
        ACE_TEXT("Exceeded Maximum number of participant keys!  ")
        ACE_TEXT("Next key will be a duplicate!\n")
      ));
    }
  
    OpenDDS::DCPS::GUID_t guid = create_guid(lastKey_);
   
    guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
   
    return guid;

  // Generate an Entity GUID value.
  } else {

    // Rudimentary validity checking.
    if( (this->lastKey_ & ~KeyMask) != 0) {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RepoIdGenerator::next: ")
        ACE_TEXT("Exceeded Maximum number of entity keys!  ")
        ACE_TEXT("Next key will be a duplicate!\n")
      ));
    }
  
    OpenDDS::DCPS::GUID_t guid = create_guid(participant_);
    
    OpenDDS::DCPS::fill_guid(guid.entityId.entityKey, lastKey_, 3);
    guid.entityId.entityKind = kindCode_;
    
    return guid;
  }
}

void
RepoIdGenerator::last( long key)
{
  if( key > this->lastKey_) {
    this->lastKey_ = key;
  }
}

OpenDDS::DCPS::GUID_t
RepoIdGenerator::create_guid(long participant)
{
  OpenDDS::DCPS::GUID_t guid = OpenDDS::DCPS::create_empty_guid();
  
  OpenDDS::DCPS::fill_guid(guid.guidPrefix + 4, federation_, 4);
  OpenDDS::DCPS::fill_guid(guid.guidPrefix + 8, participant, 4);

  return guid;
}
