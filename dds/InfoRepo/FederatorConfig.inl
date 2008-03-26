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

