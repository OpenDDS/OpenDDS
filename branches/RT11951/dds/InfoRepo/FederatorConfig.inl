// -*- C++ -*-
//
// $Id$

ACE_INLINE
int&
OpenDDS::Federator::Config::argc()
{
  return this->argc_;
}

ACE_INLINE
int
OpenDDS::Federator::Config::argc() const
{
  return this->argc_;
}

ACE_INLINE
char**&
OpenDDS::Federator::Config::argv()
{
  return this->argv_;
}

ACE_INLINE
char**
OpenDDS::Federator::Config::argv() const
{
  return this->argv_;
}

ACE_INLINE
OpenDDS::Federator::RepoKey&
OpenDDS::Federator::Config::federationId()
{
  return this->federationId_;
}

ACE_INLINE
OpenDDS::Federator::RepoKey
OpenDDS::Federator::Config::federationId() const
{
  return this->federationId_;
}

ACE_INLINE
short&
OpenDDS::Federator::Config::federationPort()
{
  return this->federationPort_;
}

ACE_INLINE
short
OpenDDS::Federator::Config::federationPort() const
{
  return this->federationPort_;
}

ACE_INLINE
std::string&
OpenDDS::Federator::Config::defaultRoute()
{
  return this->defaultRoute_;
}

ACE_INLINE
std::string
OpenDDS::Federator::Config::defaultRoute() const
{
  return this->defaultRoute_;
}

ACE_INLINE
const OpenDDS::Federator::Config::HostToRouteMap&
OpenDDS::Federator::Config::route() const
{
  return this->route_;
}

