/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportReceiveListener.h"
#include "ReceivedDataSample.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
ReceiveListenerSet::ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "ReceiveListenerSet", 6);
}

ACE_INLINE
ReceiveListenerSet::ReceiveListenerSet(const ReceiveListenerSet& rhs)
  : RcObject<ACE_SYNCH_MUTEX>()
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
                           TransportReceiveListener* listener)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "insert", 6);
  GuardType guard(this->lock_);
  MapType::iterator iter = map_.find(subscriber_id);
  if (iter != map_.end()) {
    if (!listener && iter->second) {
      // subscriber_id is already in the map with a non-null listener,
      // and this call to insert() is trying to make it null.
      return 1; // 1 ==> key already existed in map
    } else if (listener && !iter->second) {
      // subscriber_id is in the map with a null listener, update it.
      iter->second = listener;
      return 1;
    }
  }
  return OpenDDS::DCPS::bind(map_, subscriber_id, listener);
}

ACE_INLINE int
ReceiveListenerSet::remove(RepoId subscriber_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "remove", 6);
  GuardType guard(this->lock_);

  if (unbind(map_, subscriber_id) != 0) {
    ACE_ERROR_RETURN((LM_DEBUG,
                      "(%P|%t) subscriber_id (%d) not found in map_.\n",
                      subscriber_id),
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

ACE_INLINE void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received", 6);

  GuardType guard(this->lock_);

  char* ptr = sample.sample_ ? sample.sample_->rd_ptr() : 0;

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {

    if (itr->second == 0) continue; // invalid listener

    if (ptr) {
      // reset read pointer because demarshal (in data_received()) moves it.
      sample.sample_->rd_ptr(ptr);
    }
    itr->second->data_received(sample);
  }
}

ACE_INLINE void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample,
                                  const RepoId& readerId)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received(sample, readerId)", 6);
  GuardType guard(this->lock_);
  MapType::iterator itr = map_.find(readerId);
  if (itr != map_.end() && itr->second) {
    itr->second->data_received(sample);
  }
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
