/*
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

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  const bool customHeader =
    DataSampleHeader::test_flag(CONTENT_FILTER_FLAG, sample->get_sample());
#endif

  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
  }

  if (map_copy.size()) {
    TransportSendElement* send_element = new TransportSendElement(static_cast<int>(map_copy.size()), sample);
    for (MapType::iterator itr = map_copy.begin(); itr != map_copy.end(); ++itr) {

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

        Message_Block_Ptr mb (send_element->msg()->duplicate());

        DataSampleHeader::add_cfentries(guids, mb.get());

        TransportCustomizedElement* tce = new TransportCustomizedElement(send_element);
        tce->set_msg(OPENDDS_MOVE_NS::move(mb)); // tce now owns ACE_Message_Block chain

        itr->second->send(tce);

      } else {
#endif /* OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE */

        // Tell the DataLink to send it.
        itr->second->send(send_element);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
      }
#endif
    }
  } else if (sample->get_send_listener()) {
    sample->get_send_listener()->data_dropped(sample, true);
  }
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_control(DataSampleElement* sample)
{
  DBG_ENTRY_LVL("DataLinkSet", "send_control", 6);
  VDBG((LM_DEBUG, "(%P|%t) DBG: DataLinkSet::send_control %@.\n", sample));
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
  }

  TransportSendControlElement* send_element =
    new TransportSendControlElement(static_cast<int>(map_copy.size()), sample);

  for (MapType::iterator itr = map_copy.begin(); itr != map_copy.end(); ++itr) {
    itr->second->send(send_element);
  }
}

ACE_INLINE OpenDDS::DCPS::SendControlStatus
OpenDDS::DCPS::DataLinkSet::send_control(GUID_t                           pub_id,
                                         const TransportSendListener_rch& listener,
                                         const DataSampleHeader&          header,
                                         Message_Block_Ptr                msg)
{
  DBG_ENTRY_LVL("DataLinkSet","send_control",6);
  //Optimized - use cached allocator.

  MapType dup_map;
  copy_map_to(dup_map);

  if (dup_map.empty()) {
    // similar to the "no links" case in TransportClient::send()
    if (DCPS_debug_level > 4) {
      const LogGuid logger(pub_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLinkSet::send_control: ")
                 ACE_TEXT("no links for publication %C, ")
                 ACE_TEXT("not sending control message.\n"),
                 logger.c_str()));
    }
    listener->control_delivered(msg);
    return SEND_CONTROL_OK;
  }

  TransportSendControlElement* const send_element =
    new TransportSendControlElement(static_cast<int>(dup_map.size()), pub_id,
                                    listener.in(), header, OPENDDS_MOVE_NS::move(msg));

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
  GUID_t pub_id,
  const DataSampleHeader& header,
  Message_Block_Ptr response)
{
  DBG_ENTRY_LVL("DataLinkSet","send_response",6);
  GuardType guard(this->lock_);

  TransportSendControlElement* const send_element =
    new TransportSendControlElement(static_cast<int>(map_.size()), pub_id,
                                    &send_response_listener_, header,
                                    OPENDDS_MOVE_NS::move(response));
  if (!send_element) return;
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
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
  }
  const MapType::iterator end = map_copy.end();
  for (MapType::iterator itr = map_copy.begin(); itr != end; ++itr) {
    if (itr->second->remove_sample(sample) == REMOVE_RELEASED) {
      return true;
    }
  }

  return false;
}

ACE_INLINE bool
OpenDDS::DCPS::DataLinkSet::remove_all_msgs(const GUID_t& pub_id)
{
  DBG_ENTRY_LVL("DataLinkSet", "remove_all_msgs", 6);
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
  }
  const MapType::iterator end = map_copy.end();
  for (MapType::iterator itr = map_copy.begin(); itr != end; ++itr) {
    itr->second->remove_all_msgs(pub_id);
  }

  return true;
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_start(DataLinkSet* in)
{
  DBG_ENTRY_LVL("DataLinkSet","send_start",6);

  typedef OPENDDS_VECTOR(DataLink_rch) SendStartVec;
  SendStartVec send_start_vec;

  {
    GuardType guard1(lock_);
    GuardType guard2(in->lock_);
    for (MapType::iterator itr = in->map_.begin(); itr != in->map_.end(); ++itr) {
      // Attempt to add the current DataLink to this set.
      int result = OpenDDS::DCPS::bind(map_, itr->first, itr->second);

      if (result == 0) {
        // We successfully added the current DataLink to this set,
        // meaning that it wasn't already a member.  We should tell
        // the DataLink about the send_start() event.
        send_start_vec.push_back(itr->second);

      } else if (result == -1) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Failed to bind data link into set.\n"));
      }

      // Note that there is a possibility that the result == 1, which
      // means that the DataLink already exists in our map_->  We skip
      // all of these cases.
    }
  }

  for (SendStartVec::iterator it = send_start_vec.begin(); it != send_start_vec.end(); ++it) {
    (*it)->send_start();
  }
}

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_stop(GUID_t repoId)
{
  DBG_ENTRY_LVL("DataLinkSet","send_stop",6);
  // Iterate over our map_ and tell each DataLink about the send_stop() event.
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
    map_.clear();
  }

  for (MapType::iterator itr = map_copy.begin(); itr != map_copy.end(); ++itr) {
    itr->second->send_stop(repoId);
  }
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

ACE_INLINE void
OpenDDS::DCPS::DataLinkSet::send_final_acks(const GUID_t& readerid)
{
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
    map_.clear();
  }

  for (MapType::iterator itr = map_copy.begin(); itr != map_copy.end(); ++itr) {
    itr->second->send_final_acks(readerid);
  }
}
