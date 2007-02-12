// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"
#include "DataLink.h"
#include "TransportSendElement.h"

ACE_INLINE void
TAO::DCPS::DataLinkSet::send(DataSampleListElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet","send",5);
  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG: DataLinkSet::send element %@.\n"
            , sample), 5);

  TransportSendElement* send_element = 0;
  //Optimized - use cached allocator.

  { // guard scope
    GuardType guard(this->lock_);

    ACE_NEW_MALLOC(send_element,
       (TransportSendElement*)sample->transport_send_element_allocator_->malloc(),
       TransportSendElement (this->map_->current_size(),
           sample,
           sample->transport_send_element_allocator_));

    MapType::ENTRY* entry;

    for (MapType::ITERATOR itr(*map_);
   itr.next(entry);
   itr.advance())
      {
  // Bump up the DataLink ref count

        // ciju: I don't see why this is necessary.
        // Since the entry itself is ref-counted as long as the
        // entry itself isn't removed, the DataLink should be safe.
        // For now commenting it out.
        //RcHandle<DataLink> data_link (entry->int_id_);

  // Tell the DataLink to send it.
  entry->int_id_->send(send_element);
      }
  }
}


ACE_INLINE TAO::DCPS::SendControlStatus
TAO::DCPS::DataLinkSet::send_control(RepoId                 pub_id,
                                     TransportSendListener* listener,
                                     ACE_Message_Block*     msg)
{
  DBG_ENTRY_LVL("DataLinkSet","send_control",5);
  //Optimized - use cached allocator.
  TransportSendControlElement* send_element = 0;

  { // guard scope
    GuardType guard(this->lock_);

    ACE_NEW_MALLOC_RETURN(send_element,
        (TransportSendControlElement*)send_control_element_allocator_.malloc(),
        TransportSendControlElement(this->map_->current_size(),
                  pub_id,
                  listener,
                  msg,
                  &send_control_element_allocator_),
        SEND_CONTROL_ERROR);



    MapType::ENTRY* entry;

    for (MapType::ITERATOR itr(*map_);
   itr.next(entry);
   itr.advance())
      {
  entry->int_id_->send_start();
  entry->int_id_->send(send_element);
  entry->int_id_->send_stop();
      }
  }

  return SEND_CONTROL_OK;
}


ACE_INLINE int
TAO::DCPS::DataLinkSet::remove_sample(const DataSampleListElement* sample,
                                      bool  dropped_by_transport)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_sample",5);
  int status = 0;

  MapType::ENTRY* entry;

  {
    GuardType guard(this->lock_);

    for (MapType::ITERATOR itr(*map_);
   itr.next(entry);
   itr.advance())
      {
  // Tell the current DataLink to remove_sample.
  if (entry->int_id_->remove_sample(sample, dropped_by_transport) != 0)
    {
      // Still go on to all of the DataLinks.  But we will make sure
      // to return the error status when we finally leave.
      status = -1;
    }
      }
  }

  return status;
}


ACE_INLINE int
TAO::DCPS::DataLinkSet::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_all_control_msgs",5);

  MapType::ENTRY* entry;

  GuardType guard(this->lock_);

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      entry->int_id_->remove_all_control_msgs(pub_id);
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
TAO::DCPS::DataLinkSet::send_start(DataLinkSet* link_set)
{
  DBG_ENTRY_LVL("DataLinkSet","send_start",5);
  MapType::ENTRY* entry;

  GuardType guard(this->lock_);

  for (MapType::ITERATOR itr(*(link_set->map_));
       itr.next(entry);
       itr.advance())
    {
      // Attempt to add the current DataLink to this set.
      int result = this->map_->bind(entry->ext_id_,entry->int_id_, &map_entry_allocator_);

      if (result == 0)
        {
          // We successfully added the current DataLink to this set,
          // meaning that it wasn't already a member.  We should tell
          // the DataLink about the send_start() event.
          entry->int_id_->send_start();
        }
      else if (result == -1)
        {
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
TAO::DCPS::DataLinkSet::send_stop()
{
  DBG_ENTRY_LVL("DataLinkSet","send_stop",5);
  // Iterate over our map_ and tell each DataLink about the send_stop() event.
  MapType::ENTRY* entry;

  GuardType guard(this->lock_);

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      entry->int_id_->send_stop();
      this->map_->unbind (entry->ext_id_, &map_entry_allocator_);
    }
}
