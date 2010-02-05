/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "DataLink.h"
#include "TransportSendElement.h"
#include "SendResponseListener.h"
#include "dds/DCPS/Util.h"

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send(DataSampleListElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet","send",6);
  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG: DataLinkSet::send element %@.\n"
            , sample), 5);

  TransportSendElement* send_element = 0;
  //Optimized - use cached allocator.

  GuardType guard(this->lock_);

  ACE_NEW_MALLOC(send_element,
                 (TransportSendElement*)sample->transport_send_element_allocator_->malloc(),
                 TransportSendElement(map_.size(),
                                      sample,
                                      sample->transport_send_element_allocator_));

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
// Bump up the DataLink ref count

    // ciju: I don't see why this is necessary.
    // Since the entry itself is ref-counted as long as the
    // entry itself isn't removed, the DataLink should be safe.
    // For now commenting it out.
    //RcHandle<DataLink> data_link (entry->int_id_);

// Tell the DataLink to send it.
    itr->second->send(send_element);
  }
}

ACE_INLINE OpenDDS::DCPS::SendControlStatus
OpenDDS::DCPS::DataLinkSet::send_control(RepoId                 pub_id,
                                         TransportSendListener* listener,
                                         ACE_Message_Block*     msg)
{
  DBG_ENTRY_LVL("DataLinkSet","send_control",6);
  //Optimized - use cached allocator.
  TransportSendControlElement* send_element = 0;

  GuardType guard(this->lock_);

  ACE_NEW_MALLOC_RETURN(send_element,
                        (TransportSendControlElement*)send_control_element_allocator_.malloc(),
                        TransportSendControlElement(map_.size(),
                                                    pub_id,
                                                    listener,
                                                    msg,
                                                    &send_control_element_allocator_),
                        SEND_CONTROL_ERROR);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->send_start();
    itr->second->send(send_element);
    itr->second->send_stop();
  }

  return SEND_CONTROL_OK;
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_response(
  RepoId pub_id,
  ACE_Message_Block* response)
{
  DBG_ENTRY_LVL("DataLinkSet","send_response",6);
  TransportSendControlElement* send_element = 0;

  SendResponseListener listener;

  GuardType guard(this->lock_);
  ACE_NEW_MALLOC(send_element,
                 (TransportSendControlElement*)send_control_element_allocator_.malloc(),
                 TransportSendControlElement(map_.size(),
                                             pub_id,
                                             &listener,
                                             response,
                                             &send_control_element_allocator_));

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->send_start();
    itr->second->send(send_element);
    itr->second->send_stop();
  }
}

ACE_INLINE int
OpenDDS::DCPS::DataLinkSet::remove_sample(const DataSampleListElement* sample,
                                          bool  dropped_by_transport)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_sample",6);
  int status = 0;

  GuardType guard(this->lock_);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    // Tell the current DataLink to remove_sample.
    if (itr->second->remove_sample(sample, dropped_by_transport) != 0) {
      // Still go on to all of the DataLinks.  But we will make sure
      // to return the error status when we finally leave.
      status = -1;
    }
  }

  return status;
}

ACE_INLINE int
OpenDDS::DCPS::DataLinkSet::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_all_control_msgs",6);

  GuardType guard(this->lock_);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->remove_all_control_msgs(pub_id);
  }

  return 0;
}

/// This will do several things, including adding to the membership
/// of the send_links_ set.  Any DataLinks added to the send_links_
/// set will be also told about the send_start() event.  Those
/// DataLinks (in the pub_links set) that are already in the
/// send_links_ set will not be told about the send_start() event
/// since they heard about it when they were inserted into the
/// send_links_ set.
ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_start(DataLinkSet* link_set)
{
  DBG_ENTRY_LVL("DataLinkSet","send_start",6);

  GuardType guard1(this->lock_);
  GuardType guard2(link_set->lock_);

  for (MapType::iterator itr = link_set->map_.begin();
       itr != link_set->map_.end();
       ++itr) {
    // Attempt to add the current DataLink to this set.
    int result = bind(map_, itr->first, itr->second);

    if (result == 0) {
      // We successfully added the current DataLink to this set,
      // meaning that it wasn't already a member.  We should tell
      // the DataLink about the send_start() event.
      itr->second->send_start();

    } else if (result == -1) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to bind data link into set.\n"));
    }

    // Note that there is a possibility that the result == 1, which
    // means that the DataLink already exists in our map_->  We skip
    // all of these cases.
  }
}

/// This will inform each DataLink in the set about the send_stop()
/// event.  It will then clear the send_links_ set.
ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_stop()
{
  DBG_ENTRY_LVL("DataLinkSet","send_stop",6);
  // Iterate over our map_ and tell each DataLink about the send_stop() event.

  GuardType guard(this->lock_);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->send_stop();
  }

  map_.clear();
}
