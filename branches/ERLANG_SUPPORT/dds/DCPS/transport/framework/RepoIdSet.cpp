// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RepoIdSet.h"


#if !defined (__ACE_INLINE__)
#include "RepoIdSet.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::RepoIdSet::~RepoIdSet()
{
  DBG_ENTRY_LVL("RepoIdSet","~RepoIdSet",6);
}

void
OpenDDS::DCPS::RepoIdSet::serialize(TAO::DCPS::Serializer & serializer)
{
  DBG_ENTRY_LVL("RepoIdSet","serialize",6);
  CORBA::ULong sz = this->size ();
  serializer << sz;

  for (MapType::iterator itr = map_.begin();
    itr != map_.end();
    ++itr)
  {
    serializer << itr->first;
  }
}


bool
OpenDDS::DCPS::RepoIdSet::exist (const RepoId& local_id,
                             bool& last)
{
  DBG_ENTRY_LVL("RepoIdSet","exist",6);

  last = true;

  RepoId remote;
  if (find(map_, local_id, remote) == -1)
  {
    return false;
  }

  last = map_.size() == 1;
  return true;
}


void 
OpenDDS::DCPS::RepoIdSet::clear ()
{
  DBG_ENTRY_LVL("RepoIdSet","clear",6);

  this->map_.clear();
}



