/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
void
OpenDDS::Federator::Config::addArg(ACE_TCHAR* arg)
{
  this->argv_[this->argc_++] = arg;
}

ACE_INLINE
void
OpenDDS::Federator::Config::federationDomain(long domain)
{
  this->federationDomain_ = domain;
}

ACE_INLINE
long
OpenDDS::Federator::Config::federationDomain() const
{
  return this->federationDomain_;
}

ACE_INLINE
void
OpenDDS::Federator::Config::federationPort(short port)
{
  this->federationPort_ = port;
}

ACE_INLINE
short
OpenDDS::Federator::Config::federationPort() const
{
  return this->federationPort_;
}

ACE_INLINE
void
OpenDDS::Federator::Config::configFile(const tstring& file)
{
  this->configFile_ = file;
}

ACE_INLINE
OpenDDS::Federator::tstring
OpenDDS::Federator::Config::configFile() const
{
  return this->configFile_;
}

ACE_INLINE
void
OpenDDS::Federator::Config::federateIor(const tstring& ior)
{
  this->federateIor_ = ior;
}

ACE_INLINE
OpenDDS::Federator::tstring
OpenDDS::Federator::Config::federateIor() const
{
  return this->federateIor_;
}

ACE_INLINE
TAO_DDS_DCPSFederationId&
OpenDDS::Federator::Config::federationId()
{
  return this->federationId_;
}

ACE_INLINE
const TAO_DDS_DCPSFederationId&
OpenDDS::Federator::Config::federationId() const
{
  return this->federationId_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
