// -*- C++ -*-
//
// $Id$

ACE_INLINE
OpenDDS::Federator::RepoKey&
OpenDDS::Federator::RemoteLink::federationId()
{
  return this->federationId_;
}

ACE_INLINE
OpenDDS::Federator::RepoKey
OpenDDS::Federator::RemoteLink::federationId() const
{
  return this->federationId_;
}

ACE_INLINE
const std::string&
OpenDDS::Federator::RemoteLink::inbound() const
{
  return this->inbound_;
}

ACE_INLINE
const std::string&
OpenDDS::Federator::RemoteLink::external() const
{
  return this->external_;
}

ACE_INLINE
void
OpenDDS::Federator::RemoteLink::addToMst()
{
  this->subscriptions_.subscribeToUpdates( this->participant_.in());
}

ACE_INLINE
void
OpenDDS::Federator::RemoteLink::removeFromMst()
{
  this->subscriptions_.unsubscribeFromUpdates( this->participant_.in());
}

