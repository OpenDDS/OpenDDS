// -*- C++ -*-
//

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
unsigned long&
Test::Options::priority()
{
  return this->priority_;
}

ACE_INLINE
unsigned long
Test::Options::priority() const
{
  return this->priority_;
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
long
Test::Options::publisherId() const
{
  return this->publisherId_;
}

ACE_INLINE
long&
Test::Options::publisherId()
{
  return this->publisherId_;
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
bool&
Test::Options::multipleInstances()
{
  return this->multipleInstances_;
}

ACE_INLINE
bool
Test::Options::multipleInstances() const
{
  return this->multipleInstances_;
}
