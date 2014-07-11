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
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidConverter.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "TransportCustomizedElement.h"
#endif

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send(DataSampleElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet", "send", 6);
  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: DataLinkSet::send element %@.\n",
            sample), 5);
  //### Debug statements to track where connection is failing
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send --> trying to LOCK lock_\n"));
  GuardType guard(this->lock_);
    //### debugging many to many test failure 2to1
      //### Debug statements to track where connection is failing
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send --> LOCKED lock_\n"));
  TransportSendElement* send_element =
    TransportSendElement::alloc(static_cast<int>(map_.size()), sample);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  const bool customHeader =
    DataSampleHeader::test_flag(CONTENT_FILTER_FLAG, sample->get_sample());
#endif

  for (MapType::iterator itr = map_.begin(); itr != map_.end(); ++itr) {

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    if (customHeader) {
      typedef std::map<DataLinkIdType, GUIDSeq_var>::iterator FilterIter;
      FilterIter fi = sample->get_filter_per_link().find(itr->first);
      GUIDSeq* guids = 0;
      if (fi != sample->get_filter_per_link().end()) {
        guids = fi->second.ptr();
      }

      VDBG_LVL((LM_DEBUG,
        "(%P|%t) DBG: DataLink %@ filtering %d subscribers.\n",
        itr->second.in(), guids ? guids->length() : 0), 5);

      ACE_Message_Block* mb = sample->get_sample()->duplicate();

      DataSampleHeader::add_cfentries(guids, mb);

      TransportCustomizedElement* tce =
        TransportCustomizedElement::alloc(send_element, false,
          sample->get_transport_customized_element_allocator());
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
OpenDDS::DCPS::DataLinkSet::send_control(RepoId                  pub_id,
                                         TransportSendListener*  listener,
                                         const DataSampleHeader& header,
                                         ACE_Message_Block*      msg)
{
  DBG_ENTRY_LVL("DataLinkSet","send_control",6);
  //Optimized - use cached allocator.
  TransportSendControlElement* send_element = 0;

  MapType dup_map;
  copy_map_to(dup_map);

  if (dup_map.empty()) {
    // similar to the "no links" case in TransportClient::send()
    if (DCPS_debug_level > 4) {
      const GuidConverter converter(pub_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLinkSet::send_control: ")
                 ACE_TEXT("no links for publication %C, ")
                 ACE_TEXT("not sending control message.\n"),
                 std::string(converter).c_str()));
    }
    listener->control_delivered(msg);
    return SEND_CONTROL_OK;
  }

  ACE_NEW_MALLOC_RETURN(send_element,
    static_cast<TransportSendControlElement*>(
      send_control_element_allocator_.malloc()),
    TransportSendControlElement(static_cast<int>(dup_map.size()),
                                pub_id,
                                listener,
                                header,
                                msg,
                                &send_control_element_allocator_),
    SEND_CONTROL_ERROR);

  for (MapType::iterator itr = dup_map.begin();
       itr != dup_map.end();
       ++itr) {
    itr->second->send_start();
    itr->second->send(send_element);
    itr->second->send_stop(pub_id);
  }

  return SEND_CONTROL_OK;
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_response(
  RepoId pub_id,
  const DataSampleHeader& header,
  ACE_Message_Block* response)
{
  DBG_ENTRY_LVL("DataLinkSet","send_response",6);
  TransportSendControlElement* send_element = 0;

  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_response --> trying to LOCK lock_\n"));

  GuardType guard(this->lock_);
  
    //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_response --> LOCKED lock_\n"));
  
  ACE_NEW_MALLOC(send_element,
    static_cast<TransportSendControlElement*>(
      send_control_element_allocator_.malloc()),
    TransportSendControlElement(static_cast<int>(map_.size()),
                                pub_id,
                                &send_response_listener_,
                                header,
                                response,
                                &send_control_element_allocator_));
  send_response_listener_.track_message();

  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->send_start();
    itr->second->send(send_element);
    itr->second->send_stop(pub_id);
  }
}

ACE_INLINE bool
OpenDDS::DCPS::DataLinkSet::remove_sample(const DataSampleElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet", "remove_sample", 6);
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::remove_sample --> trying to LOCK lock_\n"));
  GuardType guard(this->lock_);
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::remove_sample --> LOCKED lock_\n"));
  const MapType::iterator end = this->map_.end();
  for (MapType::iterator itr = this->map_.begin(); itr != end; ++itr) {

    if (itr->second->remove_sample(sample) == REMOVE_RELEASED) {
      return true;
    }
  }

  return false;
}

ACE_INLINE bool
OpenDDS::DCPS::DataLinkSet::remove_all_msgs(RepoId pub_id)
{
  DBG_ENTRY_LVL("DataLinkSet", "remove_all_msgs", 6);
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::remove_all_msgs --> trying to LOCK lock_\n"));
  GuardType guard(this->lock_);
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::remove_all_msgs --> LOCKED lock_\n"));
  const MapType::iterator end = this->map_.end();
  for (MapType::iterator itr = this->map_.begin(); itr != end; ++itr) {
    itr->second->remove_all_msgs(pub_id);
  }

  return true;
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
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_start --> trying to LOCK lock_\n"));
  GuardType guard1(this->lock_);
    //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_start --> LOCKED lock_\n"));
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
OpenDDS::DCPS::DataLinkSet::send_stop(RepoId repoId)
{
  DBG_ENTRY_LVL("DataLinkSet","send_stop",6);
  // Iterate over our map_ and tell each DataLink about the send_stop() event.
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_stop --> trying to LOCK lock_\n"));
  GuardType guard(this->lock_);
  //### debugging many to many test failure 2to1
  //ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataLinkSet::send_stop --> LOCKED lock_\n"));
  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    itr->second->send_stop(repoId);
  }

  map_.clear();
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::copy_map_to(MapType& target)
{
  target.clear();

  // Lock the existing map
  GuardType guard(this->lock_);

  // Copy to target
  for (MapType::iterator itr = map_.begin();
       itr != map_.end();
       ++itr) {
    target.insert(*itr);
  }
}
