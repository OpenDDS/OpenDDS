/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "DataLink.h"
#include "TransportSendElement.h"
#include "SendResponseListener.h"
#include "dds/DCPS/Util.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "TransportCustomizedElement.h"
#endif

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send(DataSampleListElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet", "send", 6);
  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: DataLinkSet::send element %@.\n",
            sample), 5);

  GuardType guard(this->lock_);
  TransportSendElement* send_element =
    TransportSendElement::alloc(map_.size(), sample);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  enum { DATA, DB, MB, N_ALLOC }; // for ACE_Allocators below
  const bool customHeader =
    DataSampleHeader::test_flag(CONTENT_FILTER_FLAG, sample->sample_);
  const bool swap = (TAO_ENCAP_BYTE_ORDER !=
    DataSampleHeader::test_flag(BYTE_ORDER_FLAG, sample->sample_));
#endif

  for (MapType::iterator itr = map_.begin(); itr != map_.end(); ++itr) {

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    if (customHeader) {
      typedef std::map<DataLinkIdType, GUIDSeq_var>::iterator FilterIter;
      FilterIter fi = sample->filter_per_link_.find(itr->first);
      GUIDSeq* guids = 0;
      if (fi != sample->filter_per_link_.end()) {
        guids = fi->second.ptr();
      }

      VDBG_LVL((LM_DEBUG,
        "(%P|%t) DBG: DataLink %@ filtering %d subscribers.\n",
        itr->second.in(), guids ? guids->length() : 0), 5);

      ACE_Message_Block* mb = sample->sample_->duplicate();
      ACE_Allocator* allocators[N_ALLOC];
      mb->access_allocators(allocators[DATA], allocators[DB], allocators[MB]);
      ACE_Message_Block* optHdr;
      ACE_NEW_MALLOC(optHdr,
        static_cast<ACE_Message_Block*>(
          allocators[MB]->malloc(sizeof(ACE_Message_Block))),
        ACE_Message_Block(guids ? gen_find_size(*guids) : sizeof(CORBA::ULong),
                          ACE_Message_Block::MB_DATA,
                          0, // cont
                          0, // data
                          0, // data allocator: leave as default
                          0, // locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          allocators[DB],
                          allocators[MB]));

      Serializer ser(optHdr, swap);
      if (guids) {
        ser << *guids;
      } else {
        ser << CORBA::ULong(0);
      }

      // New chain: mb (DataSampleHeader), optHdr (GUIDSeq), data (Foo)
      optHdr->cont(mb->cont());
      mb->cont(optHdr);

      TransportCustomizedElement* tce =
        TransportCustomizedElement::alloc(send_element);
      tce->set_msg(mb); // tce now owns ACE_Message_Block chain

      itr->second->send(tce);

    } else {
#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

      // Tell the DataLink to send it.
      itr->second->send(send_element);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    }
#endif
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
    static_cast<TransportSendControlElement*>(
      send_control_element_allocator_.malloc()),
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
    static_cast<TransportSendControlElement*>(
      send_control_element_allocator_.malloc()),
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

  GuardType guard(this->lock_);
  TransportSendElement element (0, sample);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {

    // Tell the current DataLink to remove_sample.
    if (itr->second->remove_sample(element, dropped_by_transport) == 0
    && element.released ()) {
      return 0;
    }
    // else still go on to rest DataLinks.
  }

  return -1;
}

ACE_INLINE int
OpenDDS::DCPS::DataLinkSet::remove_all_msgs(RepoId pub_id)
{
  DBG_ENTRY_LVL("DataLinkSet","remove_all_msgs",6);

  GuardType guard(this->lock_);

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->remove_all_msgs(pub_id);
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
    int result = OpenDDS::DCPS::bind(map_, itr->first, itr->second);

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
