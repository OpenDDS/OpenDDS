/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImpl.h"
#include "DataLinkSetMap.h"
#include "DataLinkSet.h"
#include "DataLink.h"

#include "EntryExit.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/Util.h"

OpenDDS::DCPS::DataLinkSetMap::DataLinkSetMap()
{
  DBG_ENTRY_LVL("DataLinkSetMap","DataLinkSetMap",6);
}

OpenDDS::DCPS::DataLinkSetMap::~DataLinkSetMap()
{
  DBG_ENTRY_LVL("DataLinkSetMap","~DataLinkSetMap",6);
}

void
OpenDDS::DCPS::DataLinkSetMap::dump()
{
  GuardType guard(this->map_lock_);

  for (MapType::const_iterator current = this->map_.begin();
       current != this->map_.end();
       ++current) {
    RepoIdConverter converter(current->first);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLinkSetMap::dump() - ")
               ACE_TEXT("contains link for %C.\n"),
               std::string(converter).c_str()));
  }
}

OpenDDS::DCPS::DataLinkSet*
OpenDDS::DCPS::DataLinkSetMap::find_or_create_set(RepoId id)
{
  DBG_ENTRY_LVL("DataLinkSetMap","find_or_create_set",6);
  DataLinkSet_rch link_set;

  GuardType guard(this->map_lock_);

  if (find(this->map_, id, link_set) != 0) {
    // It wasn't found.  Create one and insert it.
    link_set = new DataLinkSet();

    if (bind(map_, id, link_set) != 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to insert new DataLinkSet into "
                 "the DataLinkSetMap.\n"));
      // Return a 'nil' DataLinkSet*
      return 0;
    }
  }

  return link_set._retn();
}

OpenDDS::DCPS::DataLinkSet*
OpenDDS::DCPS::DataLinkSetMap::find_set(RepoId id)
{
  DBG_ENTRY_LVL("DataLinkSetMap","find_set",6);
  DataLinkSet_rch link_set;
  GuardType guard(this->map_lock_);

  if (find(this->map_, id, link_set) != 0) {
    return 0;
  }

  return link_set._retn();
}

OpenDDS::DCPS::DataLinkSet*
OpenDDS::DCPS::DataLinkSetMap::find_set(RepoId id,
                                        const RepoId* remoteIds,
                                        const CORBA::ULong num_targets)
{
  DBG_ENTRY_LVL("DataLinkSetMap","find_set",6);
  DataLinkSet_rch link_set;
  GuardType guard(this->map_lock_);

  if (find(this->map_, id, link_set) != 0) {
    return 0;
  }

  DataLinkSet_rch selected_links = link_set->select_links(remoteIds, num_targets);

  return selected_links._retn();
}

/// REMEMBER: This really means find_or_create_set_then_insert_link()
int
OpenDDS::DCPS::DataLinkSetMap::insert_link(RepoId id, DataLink* link)
{
  DBG_ENTRY_LVL("DataLinkSetMap","insert_link",6);

  DataLinkSet_rch link_set = 0;
  link_set = find_or_create_set(id);

  // Now we can attempt to insert the DataLink into the DataLinkSet.
  return link_set->insert_link(link);

}

void
OpenDDS::DCPS::DataLinkSetMap::release_reservations
(ssize_t         num_remote_ids,
 const RepoId*   remote_ids,
 const RepoId    local_id,
 DataLinkSetMap& released_locals,
 const bool pub_side)
{
  DBG_ENTRY_LVL("DataLinkSetMap","release_reservations",6);
  // Note: The keys are known to always represent "remote ids" in this
  //       context.  The released map represents released "local id" to
  //       DataLink associations that result from removing the remote ids
  //       (the keys) here.
  GuardType guard(this->map_lock_);

  for (ssize_t i = 0; i < num_remote_ids; ++i) {
    DataLinkSet_rch link_set;

    if (find(this->map_, remote_ids[i], link_set) != 0) {
      RepoIdConverter remote_converter(remote_ids[i]);
      RepoIdConverter local_converter(local_id);
      VDBG((LM_WARNING,
            ACE_TEXT("(%P|%t) DataLinkSetMap::release_reservations: ")
            ACE_TEXT("failed to find remote_id %C ")
            ACE_TEXT("in map for local_id %C. Skipping this remote_id.\n"),
            std::string(remote_converter).c_str(),
            std::string(local_converter).c_str()));
      continue;
    }

    // find the link has the local/remote id association.
    DataLink_rch link = link_set->find_link(remote_ids[i], local_id, pub_side);

    if (link_set->empty()) {
      if (unbind(map_, remote_ids[i]) != 0) {
        RepoIdConverter converter(remote_ids[i]);
        VDBG((LM_DEBUG,
              ACE_TEXT("(%P|%t) WARNING: DataLinkSetMap::release_reservations: ")
              ACE_TEXT("failed to unbind remote_id %C ")
              ACE_TEXT("from map. Skipping this remote_id.\n"),
              std::string(converter).c_str()));

        continue;
      }
    }

    // guard release scope
    guard.release();  //release guard before DataLinkSet call

    // Invoke release_reservations() on the DataLink.  This can cause the
    // released_locals map to be updated with local_id to DataLink "associations"
    // that become invalid as a result of these reservation releases on
    // behalf of the remote_id.
    link->release_reservations(remote_ids[i], local_id, released_locals);

    guard.acquire();
  }
}

void
OpenDDS::DCPS::DataLinkSetMap::release_all_reservations()
{
  DBG_ENTRY_LVL("DataLinkSetMap","release_all_reservations",6);
  // TBD SOON - IMPLEMENT DataLinkSetMap::release_all_reservations()
}

// This method is called in the context that the released collection
// contains local ids (as the key type) to set of DataLinks that
// are no longer associated with the local id.
//
// Also, in this context, *this* DataLinkSetMap is the local_set_map_
// in some TransportInterface object.  The local_set_map_ contains
// local_id to DataLinkSet elements.
//
// Thus this method needs to perform a "set subtraction" operation, removing
// released DataLinks from the appropriate DataLinkSet objects in our map_.
// If this results in any of our map_'s DataLinkSet objects to become empty,
// we will remove the DataLinkSet (and the corresponding local_id) from our
// map_.
void
OpenDDS::DCPS::DataLinkSetMap::remove_released
(const DataLinkSetMap& released_locals)
{
  DBG_ENTRY_LVL("DataLinkSetMap","remove_released",6);
  // Iterate over the released_locals map where each entry in the map
  // represents a local_id and its set of DataLinks that have become invalid
  // due to the releasing of remote_id reservations from the DataLinks.
  GuardType guard(this->map_lock_);

  for (MapType::const_iterator itr = released_locals.map_.begin();
       itr != released_locals.map_.end();
       ++itr) {
    RepoId local_id = itr->first;

    DataLinkSet_rch link_set;

    // Find the DataLinkSet in our map_ that has the local_id as the key.
    if (find(map_, local_id, link_set) != 0) {
      RepoIdConverter converter(local_id);
      VDBG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataLinkSetMap::remove_released: ")
            ACE_TEXT("released local_id %C is not associated with ")
            ACE_TEXT("any DataLinkSet in map. Skipping local_id.\n"),
            std::string(converter).c_str()));
      continue;
    }

    { // guard release scope
      // Temporarily release lock for DataLinkSet invocation
      guard.release();

      // Now we can have link_set remove all members found in
      // entry->int_id_ (the released DataLinkSet for the local_id).
      // The remove_links() performs "set subtraction" logic, removing
      // the supplied set of DataLinks from the link_set.  This method
      // will return the size of the set following the removal operation.
      if (link_set->remove_links(itr->second.in()) == 0) {
        guard.acquire();  // acquire map lock before map_ invocation
        // The link_set has become empty.  Remove the entry from our map_.
        if (unbind(map_, local_id) != 0) {
          // This really shouldn't happen since we did just find the
          // link_set in the map_ using the key just a few steps earlier.

          // Just issue a warning.
          RepoIdConverter converter(local_id);
          VDBG((LM_DEBUG,
                ACE_TEXT("(%P|%t) DataLinkSetMap:remove_released: ")
                ACE_TEXT("failed to unbind released local_id %C ")
                ACE_TEXT("from the map.\n"),
                std::string(converter).c_str()));
        }

        continue; // This prevents the deadlock from trying to acquire lock twice
      }

      guard.acquire();
    }
  }
}

void
OpenDDS::DCPS::DataLinkSetMap::clear()
{
  DBG_ENTRY_LVL("DataLinkSetMap","clear",6);
  GuardType guard(this->map_lock_);
  this->map_.clear();
}
