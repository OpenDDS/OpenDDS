/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

ACE_INLINE
OpenDDS::Federator::RepoKey&
OpenDDS::Federator::ManagerImpl::id()
{
  return this->config_.federationId();
}

ACE_INLINE
OpenDDS::Federator::RepoKey
OpenDDS::Federator::ManagerImpl::id() const
{
  return this->config_.federationId();
}

ACE_INLINE
TAO_DDS_DCPSInfo_i*&
OpenDDS::Federator::ManagerImpl::info()
{
  return this->info_;
}

ACE_INLINE
TAO_DDS_DCPSInfo_i*
OpenDDS::Federator::ManagerImpl::info() const
{
  return this->info_;
}

ACE_INLINE
void
OpenDDS::Federator::ManagerImpl::localRepo(::OpenDDS::DCPS::DCPSInfo_ptr repo)
{
  this->localRepo_ = OpenDDS::DCPS::DCPSInfo::_duplicate(repo);
}

ACE_INLINE
CORBA::ORB_ptr
OpenDDS::Federator::ManagerImpl::orb()
{
  return this->orb_.ptr();
}

ACE_INLINE
void
OpenDDS::Federator::ManagerImpl::orb(CORBA::ORB_ptr value)
{
  this->orb_ = CORBA::ORB::_duplicate(value);
}
