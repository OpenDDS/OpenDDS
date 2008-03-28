// -*- C++ -*-
//
// $Id$

#include "FederatorSubscriptions.h"
#include "FederatorLinkListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#if !defined (__ACE_INLINE__)
# include "FederatorSubscriptions.inl"
#endif /* ! __ACE_INLINE__ */

namespace OpenDDS { namespace Federator {

Subscriptions::Subscriptions()
 : readers_( 4)
{
}

Subscriptions::~Subscriptions()
{
  // There is only a single subscriber, which we can grab from any of the
  // readers.
  if( 0 == CORBA::is_nil(this->readers_[0])) {
    ::DDS::Subscriber_var subscriber = this->readers_[0]->get_subscriber();

    for( unsigned int index = 0; index < this->readers_.size(); ++index) {
      // This deletes the current readers copy of the listener.  When the
      // last reader using the listener does this, the listener itself is
      // deleted.
      this->readers_[ index]->set_listener(
        ::DDS::DataReaderListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_KIND_MASK
      );
    }
  }
}

void
Subscriptions::initialize(
  ::DDS::Subscriber_ptr        subscriber,
  ::DDS::DomainParticipant_ptr participant,
  FederatorManager*            manager
)
{
  this->subscriber_ = subscriber;
  this->manager_    = manager;

  // Create the LinkState listener
  FederatorLinkListener* listener = new FederatorLinkListener( *manager);

  // Create the LinkState Topic
  // Create the LinkState subscription
}

void
Subscriptions::subscribeToUpdates( ::DDS::DomainParticipant_ptr participant)
{
  // Create the Update Listener
  // Create the Update Topics
  // Create the Update subscriptions
}

void
Subscriptions::unsubscribeFromUpdates( ::DDS::DomainParticipant_ptr participant)
{
  // set_listeners() to nil

  // Delete the updateReaders.
}

}} // End namespace OpenDDS::Federator

