/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

ACE_INLINE
OpenDDS::DCPS::PriorityKey::PriorityKey()
  : priority_(0)
{
}

ACE_INLINE
OpenDDS::DCPS::PriorityKey::PriorityKey(CORBA::Long priority, ACE_INET_Addr address)
  : priority_(priority), address_(address)
{
}

ACE_INLINE
bool
OpenDDS::DCPS::PriorityKey::operator<(const PriorityKey& rhs) const
{
  return (this->address_ < rhs.address_)? true:
         (rhs.address_ < this->address_)? false:
         this->priority_ < rhs.priority_;
}

ACE_INLINE
bool
OpenDDS::DCPS::PriorityKey::operator==(const PriorityKey& rhs) const
{
  return (this->priority_ == rhs.priority_)
         && (this->address_ == rhs.address_);
}

ACE_INLINE
unsigned long
OpenDDS::DCPS::PriorityKey::hash() const
{
  return (this->priority_ << 16) + this->address_.hash();
}

ACE_INLINE
CORBA::Long&
OpenDDS::DCPS::PriorityKey::priority()
{
  return this->priority_;
}

ACE_INLINE
CORBA::Long
OpenDDS::DCPS::PriorityKey::priority() const
{
  return this->priority_;
}

ACE_INLINE
ACE_INET_Addr&
OpenDDS::DCPS::PriorityKey::address()
{
  return this->address_;
}

ACE_INLINE
ACE_INET_Addr
OpenDDS::DCPS::PriorityKey::address() const
{
  return this->address_;
}
