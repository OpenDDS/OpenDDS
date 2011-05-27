// -*- C++ -*-
//
// $Id$

ACE_INLINE
bool&
Test::Options::verbose()
{
  return this->verbose_;
}

ACE_INLINE
bool
Test::Options::verbose() const
{
  return this->verbose_;
}

ACE_INLINE
unsigned long&
Test::Options::domain()
{
  return this->domain_;
}

ACE_INLINE
unsigned long
Test::Options::domain() const
{
  return this->domain_;
}

ACE_INLINE
long&
Test::Options::duration()
{
  return this->duration_;
}

ACE_INLINE
long
Test::Options::duration() const
{
  return this->duration_;
}

ACE_INLINE
Test::Options::TransportType
Test::Options::transportType() const
{
  return this->transportType_;
}

ACE_INLINE
Test::Options::TransportType&
Test::Options::transportType()
{
  return this->transportType_;
}

ACE_INLINE
unsigned int
Test::Options::transportKey() const
{
  return this->transportKey_;
}

ACE_INLINE
unsigned int&
Test::Options::transportKey()
{
  return this->transportKey_;
}

ACE_INLINE
std::string
Test::Options::topicName() const
{
  return this->topicName_;
}

ACE_INLINE
std::string&
Test::Options::topicName()
{
  return this->topicName_;
}

ACE_INLINE
const Test::Options::ProfileContainer&
Test::Options::profiles() const
{
  return this->publicationProfiles_;
}

