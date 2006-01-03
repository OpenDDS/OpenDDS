// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"
#include  "DataLink.h"
#include  "TransportSendElement.h"

ACE_INLINE int
TAO::DCPS::DataLinkSet::insert_link(DataLink* link)
{
  DBG_ENTRY("DataLinkSet","insert_link");
  link->_add_ref();
  DataLink_rch mylink = link;

  return this->map_->bind(mylink->id(), mylink, &map_entry_allocator_);
}


// Perform "set subtraction" logic.  Subtract the released_set from
// *this* set.  When complete, return the (new) size of the set.
ACE_INLINE ssize_t
TAO::DCPS::DataLinkSet::remove_links(DataLinkSet* released_set)
{
  DBG_ENTRY("DataLinkSet","remove_links");
  MapType::ENTRY* entry;

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
  return this->map_->current_size();
}


ACE_INLINE void
TAO::DCPS::DataLinkSet::release_reservations(RepoId          remote_id,
                                             DataLinkSetMap& released_locals)
{
  DBG_ENTRY("DataLinkSet","release_reservations");
  // Simply iterate over our set of DataLinks, and ask each one to perform
  // the release_reservations operation upon itself.
  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      entry->int_id_->release_reservations(remote_id, released_locals);
    }
}

ACE_INLINE void
TAO::DCPS::DataLinkSet::send(DataSampleListElement* sample)
{
  DBG_ENTRY("DataLinkSet","send");
  TransportSendElement* send_element = 0;
  //Optimized - use cached allocator.
  ACE_NEW_MALLOC(send_element,
                 (TransportSendElement*)sample->transport_send_element_allocator_->malloc(),
                 TransportSendElement(this->map_->current_size(),
                                      sample,
                                      sample->transport_send_element_allocator_));
                        

  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      // Tell the DataLink to send it.
      entry->int_id_->send(send_element);
    }
}


ACE_INLINE TAO::DCPS::SendControlStatus
TAO::DCPS::DataLinkSet::send_control(RepoId                 pub_id,
                                     TransportSendListener* listener,
                                     ACE_Message_Block*     msg)
{
  DBG_ENTRY("DataLinkSet","send_control");
  //Optimized - use cached allocator.
  TransportSendControlElement* send_element = 0;

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

  return SEND_CONTROL_OK;
}


ACE_INLINE int
TAO::DCPS::DataLinkSet::remove_sample(const DataSampleListElement* sample)
{
  DBG_ENTRY("DataLinkSet","remove_sample");
  int status = 0;

  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      // Tell the current DataLink to remove_sample.
      if (entry->int_id_->remove_sample(sample) != 0)
        {
          // Still go on to all of the DataLinks.  But we will make sure
          // to return the error status when we finally leave.
          status = -1;
        }
    }

  return status;
}


ACE_INLINE int
TAO::DCPS::DataLinkSet::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY("DataLinkSet","remove_all_control_msgs");

  MapType::ENTRY* entry;

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
  DBG_ENTRY("DataLinkSet","send_start");
  MapType::ENTRY* entry;

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
  DBG_ENTRY("DataLinkSet","send_stop");
  // Iterate over our map_ and tell each DataLink about the send_stop() event.
  MapType::ENTRY* entry;

  for (MapType::ITERATOR itr(*map_);
       itr.next(entry);
       itr.advance())
    {
      entry->int_id_->send_stop();
      this->map_->unbind (entry->ext_id_, &map_entry_allocator_);
    }
}

