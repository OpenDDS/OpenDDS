// -*- C++ -*-
//
// $Id$
#include  "RepoIdSet.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::RepoIdSetMap::RepoIdSetMap()
{
  DBG_ENTRY_LVL("RepoIdSetMap","RepoIdSetMap",5);
}


ACE_INLINE TAO::DCPS::RepoIdSet*
TAO::DCPS::RepoIdSetMap::find(RepoId key)
{
  DBG_ENTRY_LVL("RepoIdSetMap","find",5);
  RepoIdSet_rch value;

  if (this->map_.find(key, value) != 0)
    {
      return 0;
    }

  return value._retn();
}


ACE_INLINE size_t
TAO::DCPS::RepoIdSetMap::size() const
{
  DBG_ENTRY_LVL("RepoIdSetMap","size",5);
  return this->map_.current_size();
}


ACE_INLINE TAO::DCPS::RepoIdSet*
TAO::DCPS::RepoIdSetMap::find_or_create(RepoId key)
{
  DBG_ENTRY_LVL("RepoIdSetMap","find_or_create",5);
  RepoIdSet_rch value;

  if (this->map_.find(key, value) != 0)
    {
      // It wasn't found.  Create one and insert it.
      value = new RepoIdSet();

      if (this->map_.bind(key, value) != 0)
        {
           ACE_ERROR((LM_ERROR,
                      "(%P|%t) ERROR: Unable to insert new RepoIdSet into "
                      "the RepoIdSetMap.\n"));
           // Return a 'nil' RepoIdSet*
           return 0;
        }
    }

  return value._retn();
}


ACE_INLINE TAO::DCPS::RepoIdSetMap::MapType&
TAO::DCPS::RepoIdSetMap::map()
{
  DBG_SUB_ENTRY("RepoIdSetMap","map",1);
  return this->map_;
}


ACE_INLINE const TAO::DCPS::RepoIdSetMap::MapType&
TAO::DCPS::RepoIdSetMap::map() const
{
  DBG_SUB_ENTRY("RepoIdSetMap","map",2);
  return this->map_;
}


ACE_INLINE size_t
TAO::DCPS::RepoIdSetMap::marshaled_size ()
{
  DBG_ENTRY_LVL("RepoIdSetMap","marshaled_size",5);

  // serialize len for the map size and set size information.
  size_t size = (this->size () + 1) * sizeof (size_t);

  size_t num_ids = 0;
  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(this->map_);
    itr.next(entry);
    itr.advance())
  {
    // one sub id
    ++ num_ids;
    // num of pubids in the RepoIdSet.
    num_ids += entry->int_id_->size ();
  }

  size += num_ids * sizeof (RepoId);

  return size;
}

