/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReceivedDataSample.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"

ACE_INLINE
OpenDDS::DCPS::ReceiveListenerSetMap::ReceiveListenerSetMap()
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","ReceiveListenerSetMap",6);
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet*
OpenDDS::DCPS::ReceiveListenerSetMap::find(RepoId publisher_id) const
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","find",6);
  ReceiveListenerSet_rch listener_set;

  if (OpenDDS::DCPS::find(map_, publisher_id, listener_set) != 0) {
    return 0;
  }

  return listener_set._retn();
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet*
OpenDDS::DCPS::ReceiveListenerSetMap::find_or_create(RepoId publisher_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","find_or_create",6);
  ReceiveListenerSet_rch listener_set;

  if (OpenDDS::DCPS::find(map_, publisher_id, listener_set) != 0) {
    // It wasn't found.  Create one and insert it.
    listener_set = new ReceiveListenerSet();

    if (OpenDDS::DCPS::bind(map_, publisher_id, listener_set) != 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to insert ReceiveListenerSet into the "
                 "ReceiveListenerSetMap for publisher_id %d.\n",
                 publisher_id));
      // Return a 'nil' ReceiveListenerSet*
      return 0;
    }
  }

  return listener_set._retn();
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet*
OpenDDS::DCPS::ReceiveListenerSetMap::remove_set(RepoId publisher_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","remove_set",6);
  ReceiveListenerSet_rch listener_set;

  if (unbind(map_, publisher_id, listener_set) != 0) {
    VDBG((LM_DEBUG,
          "(%P|%t) Unable to remove ReceiveListenerSet from the "
          "ReceiveListenerSetMap for id %d.\n",
          publisher_id));
    // Return a 'nil' ReceiveListenerSet*
    return 0;
  }

  return listener_set._retn();
}

ACE_INLINE ssize_t
OpenDDS::DCPS::ReceiveListenerSetMap::size() const
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","size",6);
  return map_.size();
}

ACE_INLINE int
OpenDDS::DCPS::ReceiveListenerSetMap::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","data_received",6);

  char* ptr = sample.sample_->rd_ptr();

  // Iterate over each entry in our map_.
  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    // reset read pointer because demarshal (in data_received()) moves it.
    sample.sample_->rd_ptr(ptr);
    // Deliver the sample to the set of TransportReceiveListener objects
    itr->second->data_received(sample);
  }

  return 0;
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSetMap::MapType&
OpenDDS::DCPS::ReceiveListenerSetMap::map()
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","map",6);
  return map_;
}

ACE_INLINE const OpenDDS::DCPS::ReceiveListenerSetMap::MapType&
OpenDDS::DCPS::ReceiveListenerSetMap::map() const
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","map",6);
  return map_;
}
