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
  DBG_ENTRY("DataLinkSet","DataLinkSet");
  map_ = new MapType (ACE_Allocator::instance ());
}


TAO::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY("DataLinkSet","~DataLinkSet");
  
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
}

