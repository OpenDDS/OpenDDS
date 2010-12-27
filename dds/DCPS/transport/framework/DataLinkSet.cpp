/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataLinkSet.h"
#include "DataLinkSet_rch.h"

#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/Util.h"
#include "TransportImpl.h"
#include "TransportSendListener.h"

#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */

//TBD: The number of chunks in send control cached allocator and map
//     entry allocator are hard coded for now. These values will be
//     configured when we implement the dds configurations.

/// The number of chuncks in send control cached allocator per pub/sub.
#define NUM_SEND_CONTROL_ELEMENT_CHUNKS 20

OpenDDS::DCPS::DataLinkSet::DataLinkSet()
  : send_control_element_allocator_(NUM_SEND_CONTROL_ELEMENT_CHUNKS)
{
  DBG_ENTRY_LVL("DataLinkSet","DataLinkSet",6);

  if (OpenDDS::DCPS::Transport_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataLinkSet send_control_element_allocator %x with %d chunks\n",
               &send_control_element_allocator_, NUM_SEND_CONTROL_ELEMENT_CHUNKS));
  }
}

OpenDDS::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY_LVL("DataLinkSet","~DataLinkSet",6);
}

int
OpenDDS::DCPS::DataLinkSet::insert_link(DataLink* link)
{
  DBG_ENTRY_LVL("DataLinkSet","insert_link",6);
  link->_add_ref();
  DataLink_rch mylink = link;

  GuardType guard(this->lock_);

  return OpenDDS::DCPS::bind(map_, mylink->id(), mylink);
}

// Perform "set subtraction" logic.  Subtract the released_set from
// *this* set.  When complete, return the (new) size of the set.
ssize_t
OpenDDS::DCPS::DataLinkSet::remove_links(DataLinkSet* released_set)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_links",6);

  GuardType guard1(this->lock_);
  GuardType guard2(released_set->lock_);

  // Attempt to unbind each of the DataLinks in the released_set's
  // internal map from *this* object's internal map.
  for (DataLinkSet::MapType::iterator itr = released_set->map_.begin();
       itr != released_set->map_.end();
       ++itr) {

    DataLinkIdType link_id = itr->first;

    if (unbind(map_, link_id) != 0) {
      // Just report to the log that we tried.
      VDBG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataLinkSet::remove_links: ")
            ACE_TEXT("link_id %d not found in map.\n"),
            link_id));
    }
  }

  // Return the current size of our map following all attempts to unbind().
  return map_.size();
}

//void
//OpenDDS::DCPS::DataLinkSet::release_reservations(RepoId          remote_id,
//                                            DataLinkSetMap& released_locals)
//{
//  DBG_ENTRY_LVL("DataLinkSet","release_reservations",6);
//  // Simply iterate over our set of DataLinks, and ask each one to perform
//  // the release_reservations operation upon itself.
//  MapType::ENTRY* entry;
//
//  { // guard scope
//    GuardType guard(this->lock_);
//
//    for (MapType::ITERATOR itr(*map_);
//  itr.next(entry);
//  itr.advance())
//      {
//  entry->int_id_->release_reservations(remote_id, released_locals);
//      }
//  }
//}

OpenDDS::DCPS::DataLinkSet*
OpenDDS::DCPS::DataLinkSet::select_links(const RepoId* remoteIds,
                                         const CORBA::ULong num_targets)
{
  DBG_ENTRY_LVL("DataLinkSet","select_links",6);

  DataLinkSet_rch selected_links = new DataLinkSet();

  GuardType guard(this->lock_);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    for (CORBA::ULong i = 0; i < num_targets; ++i) {
      if (itr->second->is_target(remoteIds[i])) {
        OpenDDS::DCPS::bind(selected_links->map_,
             itr->second->id(), itr->second);
        break;
      }
    }
  }

  return selected_links._retn();
}

OpenDDS::DCPS::DataLink*
OpenDDS::DCPS::DataLinkSet::find_link(const RepoId remoteId,
                                      const RepoId localId,
                                      const bool   pub_side)
{
  DBG_ENTRY_LVL("DataLinkSet","find_link",6);

  { // guard scope
    GuardType guard(this->lock_);

    for (MapType::iterator itr = map_.begin();
         itr != map_.end();
         ++itr) {
      bool last = true;

      if (itr->second->exist(remoteId, localId, pub_side, last)) {
        DataLink_rch link = itr->second;

        if (last) {
          if (unbind(map_, itr->first) != 0) {
            RepoIdConverter converter(localId);
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) DataLinkSet::find_link: ")
                       ACE_TEXT("cannot remove link for localId %C pub_side is %C.\n"),
                       std::string(converter).c_str(),
                       (pub_side? "true": "false")));
          }
        }

        return link._retn();
      }
    }
  }

  return 0;
}

bool
OpenDDS::DCPS::DataLinkSet::empty()
{
  GuardType guard(this->lock_);

  return map_.empty();
}
