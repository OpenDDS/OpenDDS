// -*- C++ -*-
//
// $Id$

#include "FederatorManagerImpl.h"

#include "ParticipantUpdateTypeSupportC.h"
#include "ParticipantUpdateTypeSupportImpl.h"
#include "TopicUpdateTypeSupportC.h"
#include "TopicUpdateTypeSupportImpl.h"
#include "SubscriptionUpdateTypeSupportC.h"
#include "SubscriptionUpdateTypeSupportImpl.h"
#include "PublicationUpdateTypeSupportC.h"
#include "PublicationUpdateTypeSupportImpl.h"

namespace OpenDDS { namespace Federator {

template<>
void
ManagerImpl::update< ParticipantUpdate>(
  ParticipantUpdate& sample,
  ::DDS::SampleInfo& /* info */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Received ParticipantUpdate data on node %d.\n"),
      this->id()
    ));
  }
  
  /// @TODO: Implement this

  // First determine if this is new data.
  if( this->remoteLink_[ sample.id.repository]->lastSeen()
      >= sample.packet
    ) {
    return;
  }

  // Indicate that we have procesed this sequence datum.
  this->remoteLink_[ sample.id.repository]->lastSeen() = sample.packet;

  // Update the repository

  // Update the mappings

  // Update the source and sequence data
}

template<>
void
ManagerImpl::update< TopicUpdate>(
  TopicUpdate&       sample,
  ::DDS::SampleInfo& /* info */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Received TopicUpdate data on node %d.\n"),
      this->id()
    ));
  }
  
  /// @TODO: Implement this

  // First determine if this is new data.
  if( this->remoteLink_[ sample.id.repository]->lastSeen()
      >= sample.packet
    ) {
    return;
  }

  // Indicate that we have procesed this sequence datum.
  this->remoteLink_[ sample.id.repository]->lastSeen() = sample.packet;

  // Update the repository

  // Update the mappings

  // Update the source and sequence data
}

template<>
void
ManagerImpl::update< SubscriptionUpdate>(
  SubscriptionUpdate& sample,
  ::DDS::SampleInfo&  /* info */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Received SubscriptionUpdate data on node %d.\n"),
      this->id()
    ));
  }
  
  /// @TODO: Implement this

  // First determine if this is new data.
  if( this->remoteLink_[ sample.id.repository]->lastSeen()
      >= sample.packet
    ) {
    return;
  }

  // Indicate that we have procesed this sequence datum.
  this->remoteLink_[ sample.id.repository]->lastSeen() = sample.packet;

  // Update the repository

  // Update the mappings

  // Update the source and sequence data
}

template<>
void
ManagerImpl::update< PublicationUpdate>(
  PublicationUpdate& sample,
  ::DDS::SampleInfo& /* info */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Received PublicationUpdate data on node %d.\n"),
      this->id()
    ));
  }
  
  /// @TODO: Implement this

  // First determine if this is new data.
  if( this->remoteLink_[ sample.id.repository]->lastSeen()
      >= sample.packet
    ) {
    return;
  }

  // Indicate that we have procesed this sequence datum.
  this->remoteLink_[ sample.id.repository]->lastSeen() = sample.packet;

  // Update the repository

  // Update the mappings

  // Update the source and sequence data
}

}} // End of OpenDDS::Federator

