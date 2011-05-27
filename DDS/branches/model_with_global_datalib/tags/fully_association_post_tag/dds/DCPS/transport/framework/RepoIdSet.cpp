// -*- C++ -*-
//
// $Id$
#include   "DCPS/DdsDcps_pch.h"
#include  "RepoIdSet.h"


#if !defined (__ACE_INLINE__)
#include "RepoIdSet.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::RepoIdSet::~RepoIdSet()
{
  DBG_ENTRY("RepoIdSet","~RepoIdSet");
}

void
TAO::DCPS::RepoIdSet::serialize(Serializer & serializer)
{
  DBG_ENTRY("RepoIdSet","serialize");
  serializer << this->size ();
  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(this->map_);
    itr.next(entry);
    itr.advance())
  {
    serializer << entry->ext_id_;
  }
}


bool 
TAO::DCPS::RepoIdSet::equal (RepoIdSet& map)
{
  DBG_ENTRY("RepoIdSet","equal");
  
  if (this->size () == map.size () && this->size () > 0)
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

