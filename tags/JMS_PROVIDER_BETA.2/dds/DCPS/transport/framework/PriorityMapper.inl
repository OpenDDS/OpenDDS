// -*- C++ -*-
//
// $Id$

ACE_INLINE
OpenDDS::DCPS::PriorityMapper::PriorityMapper( CORBA::Long priority)
 : priority_( priority)
{
}

ACE_INLINE
CORBA::Long&
OpenDDS::DCPS::PriorityMapper::priority()
{
  return this->priority_;
}

ACE_INLINE
CORBA::Long
OpenDDS::DCPS::PriorityMapper::priority() const
{
  return this->priority_;
}

