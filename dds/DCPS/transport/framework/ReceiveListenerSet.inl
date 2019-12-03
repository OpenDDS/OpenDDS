/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportReceiveListener.h"
#include "ReceivedDataSample.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
ReceiveListenerSet::ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "ReceiveListenerSet", 6);
}

ACE_INLINE
ReceiveListenerSet::ReceiveListenerSet(const ReceiveListenerSet& rhs)
  : RcObject()
  , lock_()
  , map_(rhs.map_)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "ReceiveListenerSet(rhs)", 6);
}

ACE_INLINE ReceiveListenerSet&
ReceiveListenerSet::operator=(const ReceiveListenerSet& rhs)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "operator=", 6);
  map_ = rhs.map_;
  return *this;
}


ACE_INLINE int
ReceiveListenerSet::insert(RepoId subscriber_id,
                           const TransportReceiveListener_wrch& listener)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "insert", 6);
  GuardType guard(this->lock_);

  std::pair<MapType::iterator,bool> r = map_.insert(std::make_pair(subscriber_id,listener));
  if (!r.second) {
    // subscriber_id is already in the map
    if (!r.first->second) {
      // subscriber_id is in the map with a null listener, update it.
      r.first->second = listener;
    }
    return 1; // 1 ==> key already existed in map
  }
  return 0;
}


ACE_INLINE int
ReceiveListenerSet::remove(RepoId subscriber_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "remove", 6);
  GuardType guard(this->lock_);

  if (unbind(map_, subscriber_id) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: subscriber_id (%C) not found in map_.\n",
                      LogGuid(subscriber_id).c_str()),
                     -1);
  }

  return 0;
}

ACE_INLINE void
ReceiveListenerSet::remove_all(const GUIDSeq& to_remove)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "remove_all", 6);
  GuardType guard(this->lock_);
  const CORBA::ULong len = to_remove.length();
  for (CORBA::ULong i(0); i < len; ++i) {
    unbind(map_, to_remove[i]);
  }
}

ACE_INLINE ssize_t
ReceiveListenerSet::size() const
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "size", 6);
  GuardType guard(this->lock_);
  return map_.size();
}

ACE_INLINE ReceiveListenerSet::MapType&
ReceiveListenerSet::map()
{
  return this->map_;
}

ACE_INLINE const ReceiveListenerSet::MapType&
ReceiveListenerSet::map() const
{
  return this->map_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
