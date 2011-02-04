/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RepoIdSetMap.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/Util.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "RepoIdSetMap.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::RepoIdSetMap::~RepoIdSetMap()
{
  DBG_ENTRY_LVL("RepoIdSetMap","~RepoIdSetMap",6);
}

int
OpenDDS::DCPS::RepoIdSetMap::insert(RepoId key, RepoId value)
{
  DBG_ENTRY_LVL("RepoIdSetMap","insert",6);
  RepoIdSet_rch id_set = this->find_or_create(key);

  if (id_set.is_nil()) {
    // find_or_create failure
    RepoIdConverter converter(key);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::insert: ")
                      ACE_TEXT("failed to find_or_create RepoIdSet ")
                      ACE_TEXT("for RepoId %C.\n"),
                      std::string(converter).c_str()),-1);
  }

  int result = id_set->insert_id(value, key);

  if (result == -1) {
    RepoIdConverter value_converter(value);
    RepoIdConverter key_converter(key);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::insert: ")
               ACE_TEXT("failed to insert RepoId %C ")
               ACE_TEXT("into RepoIdSet for RepoId %C.\n"),
               std::string(value_converter).c_str(),
               std::string(key_converter).c_str()));

  } else {
    // It could be already bound, but we accept it since the subscriber
    // could send the acks for the same id multiple times.

    // Success.  Leave now.
    return 0;
  }

  // Deal with possibility that the id_set just got created - just for us.
  // If so, we need to "undo" the creation.
  if (id_set->size() == 0) {
    if (unbind(map_, key) != 0) {
      RepoIdConverter converter(key);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::insert: ")
                 ACE_TEXT("failed to unbind (undo create) an empty ")
                 ACE_TEXT("RepoIdSet for RepoId %C.\n"),
                 std::string(converter).c_str()));
    }
  }

  return -1;
}

// This version removes an individual RepoId (value) from the RepoIdSet
// associated with the "key" RepoId in our map_.
int
OpenDDS::DCPS::RepoIdSetMap::remove(RepoId key,RepoId value)
{
  DBG_ENTRY_LVL("RepoIdSetMap","remove",6);
  RepoIdSet_rch id_set;

  int result = OpenDDS::DCPS::find(map_, key, id_set);

  if (result != 0) {
    // We couldn't find the id_set for the supplied key.
    RepoIdConverter converter(key);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::remove: ")
                      ACE_TEXT("unable to locate RepoIdSet for key %C.\n"),
                      std::string(converter).c_str()),-1);
  }

  // Now we can attempt to remove the value RepoId from the id_set.
  result = id_set->remove_id(value);

  if (result != 0) {
    // We couldn't find the supplied RepoId value as a member of the id_set.
    RepoIdConverter key_converter(key);
    RepoIdConverter value_converter(value);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::remove: ")
                      ACE_TEXT("RepoIdSet for key %C does not contain ")
                      ACE_TEXT("value %C.\n"),
                      std::string(key_converter).c_str(),
                      std::string(value_converter).c_str()),-1);
  }

  return 0;
}

// This version removes an entire RepoIdSet from the map_.
OpenDDS::DCPS::RepoIdSet*
OpenDDS::DCPS::RepoIdSetMap::remove_set(RepoId key)
{
  DBG_ENTRY_LVL("RepoIdSetMap","remove_set",6);
  RepoIdSet_rch value;

  if (unbind(map_, key, value) != 0) {
    if (DCPS_debug_level > 4) {
      RepoIdConverter converter(key);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) RepeIdSetMap::remove_set: ")
                 ACE_TEXT("RepoId %C not found in map.\n"),
                 std::string(converter).c_str()));
    }

    return 0;
  }

  return value._retn();
}

int
OpenDDS::DCPS::RepoIdSetMap::release_publisher(RepoId subscriber_id,
                                               RepoId publisher_id)
{
  DBG_ENTRY_LVL("RepoIdSetMap","release_publisher",6);
  RepoIdSet_rch id_set;

  if (OpenDDS::DCPS::find(map_, subscriber_id, id_set) != 0) {
    RepoIdConverter converter(subscriber_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::release_publisher: ")
               ACE_TEXT("subscriber_id %C not found in map.\n"),
               std::string(converter).c_str()));
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

  if (id_set->size() == 0) {
    if (unbind(map_, subscriber_id) != 0) {
      RepoIdConverter converter(publisher_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::release_publisher: ")
                 ACE_TEXT("failed to remove an empty ")
                 ACE_TEXT("ReceiveListenerSet for publisher_id %C.\n"),
                 std::string(converter).c_str()));
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
OpenDDS::DCPS::RepoIdSetMap::marshal(bool byte_order)
{
  DBG_ENTRY_LVL("RepoIdSetMap","marshal",6);
  ACE_Message_Block* data = 0;

  ACE_NEW_RETURN(data,
                 ACE_Message_Block(this->marshaled_size(),
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

  Serializer writer(data, byte_order);
  CORBA::ULong sz = this->size();
  writer << sz;

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    writer << itr->first;
    itr->second->serialize(writer);
  }

  return data;
}

int
OpenDDS::DCPS::RepoIdSetMap::demarshal(ACE_Message_Block* acks, bool byte_order)
{
  DBG_ENTRY_LVL("RepoIdSetMap","demarshal",6);

  Serializer reader(acks, byte_order);

  CORBA::ULong num_subs = 0;
  reader >> num_subs;

  if (reader.good_bit() != true) return -1;

  for (CORBA::ULong i = 0; i < num_subs; ++i) {
    RepoId cur_sub = GUID_UNKNOWN;
    reader >> cur_sub;

    if (reader.good_bit() != true) return -1;

    CORBA::ULong num_pubs_per_sub = 0;
    reader >> num_pubs_per_sub;

    if (reader.good_bit() != true) return -1;

    for (CORBA::ULong j = 0; j < num_pubs_per_sub; ++j) {
      RepoId pub = GUID_UNKNOWN;
      reader >> pub;

      if (reader.good_bit() != true) return -1;

      if (this->insert(pub, cur_sub) != 0)
        return -1;
    }
  }

  return 0;
}

void
OpenDDS::DCPS::RepoIdSetMap::get_keys(RepoIdSet& keys)
{
  DBG_ENTRY_LVL("RepoIdSetMap","get_keys",6);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    keys.insert_id(itr->first, itr->first);
  }
}

void
OpenDDS::DCPS::RepoIdSetMap::operator= (const RepoIdSetMap & rh)
{
  DBG_ENTRY_LVL("RepoIdSetMap","operator=",6);
  const MapType& map = rh.map();

  for (MapType::const_iterator itr = map.begin();
       itr != map.end();
       ++itr) {
    RepoIdSet_rch set = itr->second;
    RepoIdSet::MapType& smap = set->map();

    for (RepoIdSet::MapType::iterator sitr = smap.begin();
         sitr != smap.end();
         ++sitr) {
      this->insert(itr->first, sitr->first);
    }
  }
}

void
OpenDDS::DCPS::RepoIdSetMap::clear()
{
  DBG_ENTRY_LVL("RepoIdSetMap","clear=",6);

  for (MapType::iterator itr = this->map_.begin();
       itr != this->map_.end();
       ++itr) {
    itr->second->clear();
  }

  this->map_.clear();
}

void
OpenDDS::DCPS::RepoIdSetMap::dump()
{
  DBG_ENTRY_LVL("RepoIdSetMap","dump",6);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    RepoIdSet_rch set = itr->second;

    for (RepoIdSet::MapType::iterator it = set->map().begin();
         it != set->map().end(); ++it) {
      std::stringstream buffer;
      buffer << "key  " << itr->first << " - value " << it->first;
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t)   %C \n"),
                 buffer.str().c_str()));
    }
  }
}
