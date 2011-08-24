/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RepoIdSet.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"

ACE_INLINE
OpenDDS::DCPS::RepoIdSetMap::RepoIdSetMap()
{
  DBG_ENTRY_LVL("RepoIdSetMap","RepoIdSetMap",6);
}

ACE_INLINE OpenDDS::DCPS::RepoIdSet*
OpenDDS::DCPS::RepoIdSetMap::find(RepoId key) const
{
  DBG_ENTRY_LVL("RepoIdSetMap","find",6);
  RepoIdSet_rch value;

  if (OpenDDS::DCPS::find(map_, key, value) != 0) {
    return 0;
  }

  return value._retn();
}

ACE_INLINE size_t
OpenDDS::DCPS::RepoIdSetMap::size() const
{
  DBG_ENTRY_LVL("RepoIdSetMap","size",6);
  return this->map_.size();
}

ACE_INLINE OpenDDS::DCPS::RepoIdSet*
OpenDDS::DCPS::RepoIdSetMap::find_or_create(RepoId key)
{
  DBG_ENTRY_LVL("RepoIdSetMap","find_or_create",6);
  RepoIdSet_rch value;

  if (OpenDDS::DCPS::find(map_, key, value) != 0) {
    // It wasn't found.  Create one and insert it.
    value = new RepoIdSet();

    if (OpenDDS::DCPS::bind(map_, key, value) != 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to insert new RepoIdSet into "
                 "the RepoIdSetMap.\n"));
      // Return a 'nil' RepoIdSet*
      return 0;
    }
  }

  return value._retn();
}

ACE_INLINE OpenDDS::DCPS::RepoIdSetMap::MapType&
OpenDDS::DCPS::RepoIdSetMap::map()
{
  DBG_ENTRY_LVL("RepoIdSetMap","map",6);
  return this->map_;
}

ACE_INLINE const OpenDDS::DCPS::RepoIdSetMap::MapType&
OpenDDS::DCPS::RepoIdSetMap::map() const
{
  DBG_ENTRY_LVL("RepoIdSetMap","map",6);
  return this->map_;
}

