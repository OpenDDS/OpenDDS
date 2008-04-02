// -*- C++ -*-
//
// $Id$

ACE_INLINE
::OpenDDS::Federator::RepoKey&
::OpenDDS::Federator::ManagerImpl::id()
{
  return this->config_.federationId();
}

ACE_INLINE
::OpenDDS::Federator::RepoKey
::OpenDDS::Federator::ManagerImpl::id() const
{
  return this->config_.federationId();
}

ACE_INLINE
CORBA::ORB_ptr
::OpenDDS::Federator::ManagerImpl::orb()
{
  return this->orb_.ptr();
}

ACE_INLINE
void
::OpenDDS::Federator::ManagerImpl::orb( CORBA::ORB_ptr value)
{
  this->orb_ = value;
}

