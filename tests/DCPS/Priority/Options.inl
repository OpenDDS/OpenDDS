// -*- C++ -*-
//
// $Id$

ACE_INLINE
Options::TransportType
Options::transportType() const
{
  return this->transportType_;
}

ACE_INLINE
Options::TransportType&
Options::transportType()
{
  return this->transportType_;
}

ACE_INLINE
long
Options::publisherId() const
{
  return this->publisherId_;
}

ACE_INLINE
long&
Options::publisherId()
{
  return this->publisherId_;
}

