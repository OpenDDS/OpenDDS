/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
PriorityKey::PriorityKey()
  : priority_(0), is_loopback_(false), is_active_(false)
{
}

ACE_INLINE
PriorityKey::PriorityKey(Priority priority, ACE_INET_Addr address, bool is_loopback, bool active)
  : priority_(priority), address_(address), is_loopback_(is_loopback), is_active_(active)
{
}

ACE_INLINE
bool
PriorityKey::operator<(const PriorityKey& rhs) const
{
  return (this->address_ < rhs.address_)? true:
         (rhs.address_ < this->address_)? false:
         this->priority_ < rhs.priority_? true:
         (rhs.priority_ < this->priority_)? false :
         (this->is_loopback_ != rhs.is_loopback_) ? rhs.is_loopback_ :
         (this->is_active_ != rhs.is_active_) ? rhs.is_active_ :
         false;
}

ACE_INLINE
bool
PriorityKey::operator==(const PriorityKey& rhs) const
{
  return (this->priority_ == rhs.priority_)
         && (this->address_ == rhs.address_)
         && (this->is_loopback_ == rhs.is_loopback_)
         && (this->is_active_ == rhs.is_active_);
}

ACE_INLINE
unsigned long
PriorityKey::hash() const
{
  return (this->priority_ << 16) + this->address_.hash() + this->is_loopback_
    + this->is_active_;
}

ACE_INLINE
Priority&
PriorityKey::priority()
{
  return this->priority_;
}

ACE_INLINE
Priority
PriorityKey::priority() const
{
  return this->priority_;
}

ACE_INLINE
ACE_INET_Addr&
PriorityKey::address()
{
  return this->address_;
}

ACE_INLINE
ACE_INET_Addr
PriorityKey::address() const
{
  return this->address_;
}


ACE_INLINE
bool& PriorityKey::is_loopback()
{
  return this->is_loopback_;
}


ACE_INLINE
bool PriorityKey::is_loopback() const
{
  return this->is_loopback_;
}


ACE_INLINE
bool& PriorityKey::is_active()
{
  return this->is_active_;
}


ACE_INLINE
bool PriorityKey::is_active() const
{
  return this->is_active_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
