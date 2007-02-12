// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RepoIdSetMap.h"


#if !defined (__ACE_INLINE__)
#include "RepoIdSetMap.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::RepoIdSetMap::~RepoIdSetMap()
{
  DBG_ENTRY_LVL("RepoIdSetMap","~RepoIdSetMap",5);
}


int
TAO::DCPS::RepoIdSetMap::insert(RepoId key, RepoId value)
{
  DBG_ENTRY_LVL("RepoIdSetMap","insert",5);
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

  if (result == -1)
    {

      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to insert RepoId (%d) "
                 "into RepoIdSet for RepoId (%d).\n",
                 value, key));
    }
  else
    {
      // It could be already bound, but we accept it since the subscriber
      // could send the acks for the same id multiple times.

      // Success.  Leave now.
      return 0;
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
  DBG_ENTRY_LVL("RepoIdSetMap","remove",5);
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
  DBG_ENTRY_LVL("RepoIdSetMap","remove_set",5);
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
  DBG_ENTRY_LVL("RepoIdSetMap","release_publisher",5);
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

  VDBG_LVL((LM_DEBUG, "(%P|%t) RepoId size: %d.\n", id_set->size()), 5);
  // Return 1 if set is empty, 0 if not empty.
  //return (id_set->size() == 0) ? 1 : 0;

  if (id_set->size() == 0)
    {
      if (this->map_.unbind(subscriber_id) != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to remove an empty "
                     "ReceiveListenerSet for publisher_id (%d).\n",
                     publisher_id));
        }

      // We always return 1 if we know the publisher_id is no longer
      // associated with any ReceiveListeners.
      return 1;
    }

  // There are still ReceiveListeners associated with the publisher_id.
  // We return a 0 in this case.
  return 0;

}


ACE_Message_Block*
TAO::DCPS::RepoIdSetMap::marshal (bool byte_order)
{
  DBG_ENTRY_LVL("RepoIdSetMap","marshal",5);
  ACE_Message_Block* data = 0;

  ACE_NEW_RETURN (data,
    ACE_Message_Block(this->marshaled_size (),
    ACE_Message_Block::MB_DATA,
    0, //cont
    0, //data
    0, //allocator_strategy
    0, //locking_strategy
    ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
    ACE_Time_Value::zero,
    ACE_Time_Value::max_time,
    0,
    0),
    0);

  TAO::DCPS::Serializer writer(data, byte_order);
  CORBA::ULong sz = this->size ();
  writer << sz;

  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(this->map_);
    itr.next(entry);
    itr.advance())
  {
    writer << entry->ext_id_;
    entry->int_id_->serialize (writer);
  }

  return data;
}



bool
TAO::DCPS::RepoIdSetMap::equal (RepoIdSetMap& map, RepoId id)
{
  DBG_ENTRY_LVL("RepoIdSetMap","equal",5);

  RepoIdSet_rch given_id_set = map.find (id);
  RepoIdSet_rch this_id_set = this->find (id);

  if (! given_id_set.is_nil () && ! this_id_set.is_nil ())
  {
    return this_id_set->equal (* (given_id_set.in ()));
  }

  return false;
}


int
TAO::DCPS::RepoIdSetMap::demarshal (ACE_Message_Block* acks, bool byte_order)
{
  DBG_ENTRY_LVL("RepoIdSetMap","demarshal",5);

  TAO::DCPS::Serializer reader( acks, byte_order);

  CORBA::ULong num_subs = 0;
  reader >> num_subs;
  if( reader.good_bit() != true) return -1;

  for (CORBA::ULong i = 0; i < num_subs; ++i)
  {
    RepoId cur_sub = 0;
    reader >> cur_sub;
    if( reader.good_bit() != true) return -1;
    CORBA::ULong num_pubs_per_sub = 0;
    reader >> num_pubs_per_sub;
    if( reader.good_bit() != true) return -1;

    for (CORBA::ULong j = 0; j < num_pubs_per_sub; ++j)
    {
      RepoId pub = 0;
      reader >> pub;
      if( reader.good_bit() != true) return -1;
      if (this->insert (pub, cur_sub) != 0)
        return -1;
    }
  }

  return 0;
}



void
TAO::DCPS::RepoIdSetMap::get_keys (RepoIdSet& keys)
{
  DBG_ENTRY_LVL("RepoIdSetMap","get_keys",5);

  MapType::ENTRY* entry;

    for (MapType::ITERATOR itr(this->map_);
      itr.next(entry);
      itr.advance())
    {
      keys.insert_id (entry->ext_id_);
    }
}
