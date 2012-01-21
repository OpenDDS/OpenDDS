/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RepoIdSetMap.h"
#include "dds/DCPS/GuidConverter.h"
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
    GuidConverter converter(key);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::insert: ")
                      ACE_TEXT("failed to find_or_create RepoIdSet ")
                      ACE_TEXT("for RepoId %C.\n"),
                      std::string(converter).c_str()),-1);
  }

  int result = id_set->insert_id(value, key);

  if (result == -1) {
    GuidConverter value_converter(value);
    GuidConverter key_converter(key);
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
      GuidConverter converter(key);
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
    GuidConverter converter(key);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: RepoIdSetMap::remove: ")
                      ACE_TEXT("unable to locate RepoIdSet for key %C.\n"),
                      std::string(converter).c_str()),-1);
  }

  // Now we can attempt to remove the value RepoId from the id_set.
  result = id_set->remove_id(value);

  if (result != 0) {
    // We couldn't find the supplied RepoId value as a member of the id_set.
    GuidConverter key_converter(key);
    GuidConverter value_converter(value);
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
      GuidConverter converter(key);
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
    GuidConverter converter(subscriber_id);
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
      GuidConverter converter(publisher_id);
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

void
OpenDDS::DCPS::RepoIdSetMap::get_keys(RepoIdSet& keys) const
{
  DBG_ENTRY_LVL("RepoIdSetMap","get_keys",6);

  for (MapType::const_iterator itr = map_.begin();
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
