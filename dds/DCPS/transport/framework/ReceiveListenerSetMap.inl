/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReceivedDataSample.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/GuidConverter.h"

ACE_INLINE
OpenDDS::DCPS::ReceiveListenerSetMap::ReceiveListenerSetMap()
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","ReceiveListenerSetMap",6);
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet_rch
OpenDDS::DCPS::ReceiveListenerSetMap::find(RepoId publisher_id) const
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","find",6);
  ReceiveListenerSet_rch listener_set;

  if (OpenDDS::DCPS::find(map_, publisher_id, listener_set) != 0) {
    return ReceiveListenerSet_rch();
  }
  return listener_set;
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet_rch
OpenDDS::DCPS::ReceiveListenerSetMap::find_or_create(RepoId publisher_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","find_or_create",6);

  ReceiveListenerSet_rch& listener_set = map_[publisher_id];
  if (!listener_set) {
    listener_set = make_rch<ReceiveListenerSet>();
  }
  return listener_set;
}

ACE_INLINE OpenDDS::DCPS::ReceiveListenerSet_rch
OpenDDS::DCPS::ReceiveListenerSetMap::remove_set(RepoId publisher_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","remove_set",6);
  ReceiveListenerSet_rch listener_set;

  if (unbind(map_, publisher_id, listener_set) != 0) {
    VDBG((LM_DEBUG,
          "(%P|%t) Unable to remove ReceiveListenerSet from the "
          "ReceiveListenerSetMap for id %d.\n",
          LogGuid(publisher_id).c_str()));
    // Return a 'nil' ReceiveListenerSet*
    return ReceiveListenerSet_rch();
  }

  return listener_set;
}

ACE_INLINE ssize_t
OpenDDS::DCPS::ReceiveListenerSetMap::size() const
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","size",6);
  return map_.size();
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
