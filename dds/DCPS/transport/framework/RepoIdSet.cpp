// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RepoIdSet.h"


#if !defined (__ACE_INLINE__)
#include "RepoIdSet.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::RepoIdSet::~RepoIdSet()
{
  DBG_ENTRY_LVL("RepoIdSet","~RepoIdSet",5);
}

void
TAO::DCPS::RepoIdSet::serialize(Serializer & serializer)
{
  DBG_ENTRY_LVL("RepoIdSet","serialize",5);
  CORBA::ULong sz = this->size ();
  serializer << sz;
  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(this->map_);
    itr.next(entry);
    itr.advance())
  {
    serializer << entry->ext_id_;
  }
}


bool
TAO::DCPS::RepoIdSet::is_subset (RepoIdSet& map)
{
  DBG_ENTRY_LVL("RepoIdSet","is_subset",5);

  if (this->size () <= map.size () && this->size () > 0)
  {
    MapType::ENTRY* entry;

    for (MapType::ITERATOR itr(this->map_);
      itr.next(entry);
      itr.advance())
    {
      MapType::ENTRY* ientry;
      if (map.map_.find (entry->ext_id_, ientry) != 0)
        return false;
    }

    return true;
  }

  return false;
}

bool
TAO::DCPS::RepoIdSet::exist (const RepoId& local_id,
                             bool& last)
{
  DBG_ENTRY_LVL("RepoIdSet","exist",5);

  last = true;

  RepoId remote;
  if (this->map_.find(local_id, remote) == -1)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t)RepoIdSet::exist could not find local %d "
      "in map.\n", local_id));

    return false;
  }

  last = this->map_.current_size () == 1;
  return true;
}
