// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "DataLinkSetMap.h"
#include  "DataLinkSet.h"
#include  "DataLink.h"

#include "EntryExit.h"


TAO::DCPS::DataLinkSetMap::DataLinkSetMap()
{
  DBG_ENTRY("DataLinkSetMap","DataLinkSetMap");
}


TAO::DCPS::DataLinkSetMap::~DataLinkSetMap()
{
  DBG_ENTRY("DataLinkSetMap","~DataLinkSetMap");
}


TAO::DCPS::DataLinkSet*
TAO::DCPS::DataLinkSetMap::find_or_create_set(RepoId id)
{
  DBG_ENTRY("DataLinkSetMap","find_or_create_set");
  DataLinkSet_rch link_set;

  if (this->map_.find(id, link_set) != 0)
    {
      // It wasn't found.  Create one and insert it.
      link_set = new DataLinkSet();

      if (this->map_.bind(id, link_set) != 0)
        {
           ACE_ERROR((LM_ERROR,
                      "(%P|%t) ERROR: Unable to insert new DataLinkSet into "
                      "the DataLinkSetMap.\n"));
           // Return a 'nil' DataLinkSet*
           return 0;
        }
    }

  return link_set._retn();
}


TAO::DCPS::DataLinkSet*
TAO::DCPS::DataLinkSetMap::find_set(RepoId id)
{
  DBG_ENTRY("DataLinkSetMap","find_set");
  DataLinkSet_rch link_set;

  if (this->map_.find(id, link_set) != 0)
    {
      return 0;
    }

  return link_set._retn();
}


/// REMEMBER: This really means find_or_create_set_then_insert_link()
int
TAO::DCPS::DataLinkSetMap::insert_link(RepoId id, DataLink* link)
{
  DBG_ENTRY("DataLinkSetMap","insert_link");
  DataLinkSet_rch link_set;

  // Try to find it first...
  if (this->map_.find(id, link_set) != 0)
    {
      // It wasn't found.  We have to create it.
      link_set = new DataLinkSet();

      if (this->map_.bind(id, link_set) != 0)
        {
           ACE_ERROR((LM_ERROR,
                      "(%P|%t) ERROR: Unable to insert new DataLinkSet into "
                      "the DataLinkSetMap.\n"));
           // This means we failed.  Return -1 to indicate this fact.
           return -1;
        }
    }
//MJM: So why didn't you just call find_or_create_set() here?
//MJM:   return find_or_create_set( id)->insert( link) ;

  // Now we can attempt to insert the DataLink into the DataLinkSet.
  return link_set->insert_link(link);
}


void
TAO::DCPS::DataLinkSetMap::release_reservations
                                       (ssize_t         num_remote_ids,
                                        const RepoId*   remote_ids,
                                        DataLinkSetMap& released_locals)
{
  DBG_ENTRY("DataLinkSetMap","release_reservations");
  // Note: The keys are known to always represent "remote ids" in this
  //       context.  The released map represents released "local id" to
  //       DataLink associations that result from removing the remote ids
  //       (the keys) here.
  for (ssize_t i = 0; i < num_remote_ids; ++i)
    {
      RepoId remote_id = remote_ids[i];

      DataLinkSet_rch link_set;

      if (this->map_.unbind(remote_id,link_set) != 0)
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) Warning: Failed to unbind remote_id (%d) "
                     "from map_. Skipping this remote_id.\n",
                     remote_id));

          continue;
        }

      // Ask the DataLinkSet to invoke release_reservations() on each
      // DataLink object within the set.  This can cause the released_locals
      // map to be updated with local_id to DataLink "associations" that
      // become invalid as a result of these reservation releases on
      // behalf of the remote_id.
      link_set->release_reservations(remote_id,released_locals);
    }
}


void
TAO::DCPS::DataLinkSetMap::release_all_reservations()
{
  DBG_ENTRY("DataLinkSetMap","release_all_reservations");
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
TAO::DCPS::DataLinkSetMap::remove_released
                                     (const DataLinkSetMap& released_locals)
{
  DBG_ENTRY("DataLinkSetMap","remove_released");
  // Iterate over the released_locals map where each entry in the map
  // represents a local_id and its set of DataLinks that have become invalid
  // due to the releasing of remote_id reservations from the DataLinks.
  MapType::ENTRY* entry;

  for (MapType::CONST_ITERATOR itr(released_locals.map_);
       itr.next(entry);
       itr.advance())
    {
      RepoId local_id = entry->ext_id_;

      DataLinkSet_rch link_set;

      // Find the DataLinkSet in our map_ that has the local_id as the key.
      if (this->map_.find(local_id, link_set) != 0)
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) Released local_id (%d) is not associated with "
                     "any DataLinkSet in map_. Skipping local_id.\n",
                     local_id));
          continue;
        }

      // Now we can have link_set remove all members found in
      // entry->int_id_ (the released DataLinkSet for the local_id).
      // The remove_links() performs "set subtraction" logic, removing
      // the supplied set of DataLinks from the link_set.  This method
      // will return the size of the set following the removal operation.
      if (link_set->remove_links(entry->int_id_.in()) == 0)
        {
          // The link_set has become empty.  Remove the entry from our map_.
          if (this->map_.unbind(local_id) != 0)
            {
              // This really shouldn't happen since we did just find the
              // link_set in the map_ using the key just a few steps earlier.

              // Just issue a warning.
              VDBG((LM_DEBUG,
                         "(%P|%t) Failed to unbind released local_id (%d) "
                         "from the map_.\n",
                         local_id));
            }
        }
    }
}


void
TAO::DCPS::DataLinkSetMap::clear()
{
  DBG_ENTRY("DataLinkSetMap","clear");
  this->map_.close();
}
