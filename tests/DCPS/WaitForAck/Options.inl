// -*- C++ -*-
//

ACE_INLINE
Test::Options::operator bool() const
{
  return this->valid_;
}

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
bool&
Test::Options::publisher()
{
  return this->publisher_;
}

ACE_INLINE
bool
Test::Options::publisher() const
{
  return this->publisher_;
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
long&
Test::Options::publications()
{
  return this->publications_;
}

ACE_INLINE
long
Test::Options::publications() const
{
  return this->publications_;
}

ACE_INLINE
unsigned int&
Test::Options::transportKey()
{
  return this->transportKey_;
}

ACE_INLINE
unsigned int
Test::Options::transportKey() const
{
  return this->transportKey_;
}

