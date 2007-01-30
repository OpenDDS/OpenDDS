// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "DataLinkSet.h"

#include  "dds/DCPS/DataSampleList.h"
#include  "TransportImpl.h"
#include  "TransportSendListener.h"

#include "EntryExit.h"


#if !defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */

//TBD: The number of chunks in send control cached allocator and map
//     entry allocator are hard coded for now. These values will be
//     configured when we implement the dds configurations.

/// The number of chuncks in send control cached allocator per pub/sub.
#define NUM_SEND_CONTROL_ELEMENT_CHUNKS 20

/// The map_entry_allocator_ needs allocate one chunk per datalink/datareader.
/// The number of chunks for the map entry allocator should be the number of
/// datalinks per pub/sub.
#define NUM_HASH_MAP_ENTRY_CHUNKS 20

TAO::DCPS::DataLinkSet::DataLinkSet()
  : map_entry_allocator_(NUM_HASH_MAP_ENTRY_CHUNKS),
    map_(0),
    send_control_element_allocator_(NUM_SEND_CONTROL_ELEMENT_CHUNKS)
{
  DBG_ENTRY_LVL("DataLinkSet","DataLinkSet",5);
  map_ = new MapType (ACE_Allocator::instance ());

  if (DCPS_debug_level >= 2)
    {
      ACE_DEBUG ((LM_DEBUG, "(%P|%t)DataLinkSet map_entry_allocator %x with %d chunks \n",
      &map_entry_allocator_, NUM_HASH_MAP_ENTRY_CHUNKS));
      ACE_DEBUG ((LM_DEBUG, "(%P|%t)DataLinkSet send_control_element_allocator %x with %d chunks\n",
      &send_control_element_allocator_, NUM_SEND_CONTROL_ELEMENT_CHUNKS));
    }
}


TAO::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY_LVL("DataLinkSet","~DataLinkSet",5);

  // Note the unbind_all() and close() are not needed if the
  // ACE_Hash_Map_Manager_Ex and ACE_Hash_Map_With_Allocator well
  // define the intefaces to handle seperate allocators for hash
  // table and buckets/entries.
  // We call unbind_all() and then close() to work around
  // the problem when we use seperate allocators for hash table
  // and buckets/entries, the ACE_Hash_Map_Manager will use
  // the bucket/entry allocator to try to free the hash table.

  // This unbind_all() will use the allocator for previous bind/unbind
  // operations.
  this->map_->unbind_all ();
  // Close with the default allocator since the hash map opened with
  // the default allocator.
  // Unless we pass it, the map will use the map_entry_allocator_ allocator.
  this->map_->close (ACE_Allocator::instance ());
  // Delete the hash map
  delete this->map_;

  //} // guard scope
}


int
TAO::DCPS::DataLinkSet::insert_link(DataLink* link)
{
  DBG_ENTRY_LVL("DataLinkSet","insert_link",5);
  link->_add_ref();
  DataLink_rch mylink = link;

  { // guard scope
    GuardType guard(this->lock_);

    return this->map_->bind(mylink->id(), mylink, &map_entry_allocator_);
  }
}


// Perform "set subtraction" logic.  Subtract the released_set from
// *this* set.  When complete, return the (new) size of the set.
ssize_t
TAO::DCPS::DataLinkSet::remove_links(DataLinkSet* released_set)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_links",5);
  MapType::ENTRY* entry;

  ssize_t map_size = 0;

  { // guard scope
    GuardType guard(this->lock_);

    // Attempt to unbind each of the DataLinks in the released_set's
    // internal map from *this* object's internal map.
    for (DataLinkSet::MapType::ITERATOR itr(*(released_set->map_));
   itr.next(entry);
   itr.advance())
      {
  DataLinkIdType link_id = entry->ext_id_;

  if (this->map_->unbind(link_id, &map_entry_allocator_) != 0)
    {
      //MJM: This is an excellent candidate location for a level driven
      //MJM: diagnostic (ORBDebugLevel 4).
      // Just report to the log that we tried.
      VDBG((LM_DEBUG,
      "(%P|%t) link_id (%d) not found in map_->\n",
      link_id));
    }
      }

    // Return the current size of our map following all attempts to unbind().
    map_size = this->map_->current_size();
  }

  return map_size;
}


void
TAO::DCPS::DataLinkSet::release_reservations(RepoId          remote_id,
                                             DataLinkSetMap& released_locals)
{
  DBG_ENTRY_LVL("DataLinkSet","release_reservations",5);
  // Simply iterate over our set of DataLinks, and ask each one to perform
  // the release_reservations operation upon itself.
  MapType::ENTRY* entry;

  { // guard scope
    GuardType guard(this->lock_);

    for (MapType::ITERATOR itr(*map_);
   itr.next(entry);
   itr.advance())
      {
  entry->int_id_->release_reservations(remote_id, released_locals);
      }
  }
}
