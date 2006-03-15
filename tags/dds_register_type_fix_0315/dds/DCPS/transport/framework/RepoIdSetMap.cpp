// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "RepoIdSetMap.h"


#if !defined (__ACE_INLINE__)
#include "RepoIdSetMap.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::RepoIdSetMap::~RepoIdSetMap()
{
  DBG_ENTRY("RepoIdSetMap","~RepoIdSetMap");
}


int
TAO::DCPS::RepoIdSetMap::insert(RepoId key, RepoId value)
{
  DBG_ENTRY("RepoIdSetMap","insert");
  RepoIdSet_rch id_set = this->find_or_create(key);

  if (id_set.is_nil())
    {
      // find_or_create failure
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to find_or_create RepoIdSet "
                        "for RepoId (%d).\n",
                        key),
                       -1);
    }

  int result = id_set->insert_id(value);

  if (result == 0)
    {
      // Success.  Leave now.
      return 0;
    }

  // Handle the two possible failure cases (duplicate key or unknown)
  if (result == 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: RepoId (%d) already exists "
                 "in RepoIdSet for RepoId (%d).\n",
                 value, key));
    }
  else
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to insert RepoId (%d) "
                 "into RepoIdSet for RepoId (%d).\n",
                 value, key));
    }

  // Deal with possibility that the id_set just got created - just for us.
  // If so, we need to "undo" the creation.
  if (id_set->size() == 0)
    {
      if (this->map_.unbind(key) != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to unbind (undo create) an empty "
                     "RepoIdSet for RepoId (%d).\n",
                     key));
        }
    }

  return -1;
}


// This version removes an individual RepoId (value) from the RepoIdSet
// associated with the "key" RepoId in our map_.
int
TAO::DCPS::RepoIdSetMap::remove(RepoId key,RepoId value)
{
  DBG_ENTRY("RepoIdSetMap","remove");
  RepoIdSet_rch id_set;

  int result = this->map_.find(key,id_set);

  if (result != 0)
    {
      // We couldn't find the id_set for the supplied key.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to locate RepoIdSet for key %d.\n",
                        key),
                       -1);  
    }

  // Now we can attempt to remove the value RepoId from the id_set.
  result = id_set->remove_id(value);

  if (result != 0)
    {
      // We couldn't find the supplied RepoId value as a member of the id_set.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: RepoIdSet for key %d does not contain "
                        "value %d.\n",
                        key, value),
                       -1);  
    }

  return 0;
}


// This version removes an entire RepoIdSet from the map_.
TAO::DCPS::RepoIdSet*
TAO::DCPS::RepoIdSetMap::remove_set(RepoId key)
{
  DBG_ENTRY("RepoIdSetMap","remove_set");
  RepoIdSet_rch value;

  if (this->map_.unbind(key,value) != 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) RepoId (%d) not found in map_.\n",key));
      return 0;
    }

  return value._retn();
}


int
TAO::DCPS::RepoIdSetMap::release_publisher(RepoId subscriber_id,
                                           RepoId publisher_id)
{
  DBG_ENTRY("RepoIdSetMap","release_publisher");
  RepoIdSet_rch id_set;

  if (this->map_.find(subscriber_id, id_set) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: subscriber_id (%d) not found in map_.\n",
                 subscriber_id));
      // Return 1 to indicate that the subscriber_id is no longer associated
      // with any publishers at all.
      return 1;
    }

  int result = id_set->remove_id(publisher_id);

  // Ignore the result
  ACE_UNUSED_ARG(result);

  // Return 1 if set is empty, 0 if not empty.
  return (id_set->size() == 0) ? 1 : 0;
}

