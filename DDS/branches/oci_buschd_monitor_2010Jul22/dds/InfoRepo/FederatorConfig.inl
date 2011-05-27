/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
ACE_TCHAR**&
OpenDDS::Federator::Config::argv()
{
  return this->argv_;
}

ACE_INLINE
ACE_TCHAR**
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
long&
OpenDDS::Federator::Config::federationDomain()
{
  return this->federationDomain_;
}

ACE_INLINE
long
OpenDDS::Federator::Config::federationDomain() const
{
  return this->federationDomain_;
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
OpenDDS::Federator::tstring&
OpenDDS::Federator::Config::configFile()
{
  return this->configFile_;
}

ACE_INLINE
OpenDDS::Federator::tstring
OpenDDS::Federator::Config::configFile() const
{
  return this->configFile_;
}

ACE_INLINE
OpenDDS::Federator::tstring&
OpenDDS::Federator::Config::federateIor()
{
  return this->federateIor_;
}

ACE_INLINE
OpenDDS::Federator::tstring
OpenDDS::Federator::Config::federateIor() const
{
  return this->federateIor_;
}
