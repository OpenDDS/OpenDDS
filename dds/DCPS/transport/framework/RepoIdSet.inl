/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

#include "dds/DCPS/Util.h"

ACE_INLINE
OpenDDS::DCPS::RepoIdSet::RepoIdSet()
{
  DBG_ENTRY_LVL("RepoIdSet","RepoIdSet",6);
}

ACE_INLINE int
OpenDDS::DCPS::RepoIdSet::insert_id(RepoId key, RepoId value)
{
  DBG_ENTRY_LVL("RepoIdSet","insert_id",6);
  return OpenDDS::DCPS::bind(map_, key, value);
}

ACE_INLINE int
OpenDDS::DCPS::RepoIdSet::remove_id(RepoId id)
{
  DBG_ENTRY_LVL("RepoIdSet","remove_id",6);
  int result = unbind(map_, id);

  if (result != 0) {
    VDBG((LM_DEBUG, "(%P|%t) RepoId (%d) not found in map_.\n",id));
  }

  return result;
}

ACE_INLINE size_t
OpenDDS::DCPS::RepoIdSet::size() const
{
  DBG_ENTRY_LVL("RepoIdSet","size",6);
  return map_.size();
}

ACE_INLINE OpenDDS::DCPS::RepoIdSet::MapType&
OpenDDS::DCPS::RepoIdSet::map()
{
  DBG_ENTRY_LVL("RepoIdSet","map",6);
  return this->map_;
}

ACE_INLINE const OpenDDS::DCPS::RepoIdSet::MapType&
OpenDDS::DCPS::RepoIdSet::map() const
{
  DBG_ENTRY_LVL("RepoIdSet","map",6);
  return this->map_;
}
