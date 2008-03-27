// -*- C++ -*-
//
// $Id$

ACE_INLINE
OpenDDS::Federator::RepoKey&
OpenDDS::Federator::RemoteData::federationId()
{
  return this->federationId_;
}

ACE_INLINE
OpenDDS::Federator::RepoKey
OpenDDS::Federator::RemoteData::federationId() const
{
  return this->federationId_;
}

ACE_INLINE
const std::string&
OpenDDS::Federator::RemoteData::inbound() const
{
  return this->inbound_;
}

ACE_INLINE
const std::string&
OpenDDS::Federator::RemoteData::external() const
{
  return this->external_;
}

