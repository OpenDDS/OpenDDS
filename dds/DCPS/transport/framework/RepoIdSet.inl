// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::RepoIdSet::RepoIdSet()
{
  DBG_ENTRY_LVL("RepoIdSet","RepoIdSet",5);
}



ACE_INLINE int
TAO::DCPS::RepoIdSet::insert_id(RepoId id)
{
  DBG_ENTRY_LVL("RepoIdSet","insert_id",5);
  return this->map_.bind(id, id);
}


ACE_INLINE int
TAO::DCPS::RepoIdSet::remove_id(RepoId id)
{
  DBG_ENTRY_LVL("RepoIdSet","remove_id",5);
  int result = this->map_.unbind(id);

  if (result != 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) RepoId (%d) not found in map_.\n",id));
    }

  return result;
}


ACE_INLINE size_t
TAO::DCPS::RepoIdSet::size() const
{
  DBG_ENTRY_LVL("RepoIdSet","size",5);
  return this->map_.current_size();
}


ACE_INLINE TAO::DCPS::RepoIdSet::MapType&
TAO::DCPS::RepoIdSet::map()
{
  DBG_SUB_ENTRY("RepoIdSet","map",1);
  return this->map_;
}


ACE_INLINE const TAO::DCPS::RepoIdSet::MapType&
TAO::DCPS::RepoIdSet::map() const
{
  DBG_SUB_ENTRY("RepoIdSet","map",2);
  return this->map_;
}

