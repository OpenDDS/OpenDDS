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

//TBD: The number of chunks in send control cached allocator is hard 
//     coded for now. This will be configurable when we implement the  
//     dds configurations.

/// The number of chuncks in send control cached allocator per pub/sub.
#define NUM_SEND_CONTROL_ELEMENT_CHUNKS 20

// The map_entry_allocator_ only need one chunk since it's allocated for binding
// in send_start() and released in send_stop() and the send() is synchronized.
// The allocator = 0 is passed to map constructor which creates buckets on 
// heap and the cached allocator is passed when binding and unbinding.
TAO::DCPS::DataLinkSet::DataLinkSet()
: map_entry_allocator_(1),
  map_(0), 
  send_control_element_allocator_(NUM_SEND_CONTROL_ELEMENT_CHUNKS)
{
  DBG_ENTRY("DataLinkSet","DataLinkSet");
}


TAO::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY("DataLinkSet","~DataLinkSet");
}

