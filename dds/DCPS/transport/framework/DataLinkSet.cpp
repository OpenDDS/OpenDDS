/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataLinkSet.h"
#include "DataLinkSet_rch.h"

#include "dds/DCPS/GuidConverter.h"
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
  : send_response_listener_("DataLinkSet")
{
  DBG_ENTRY_LVL("DataLinkSet","DataLinkSet",6);
}

OpenDDS::DCPS::DataLinkSet::~DataLinkSet()
{
  DBG_ENTRY_LVL("DataLinkSet","~DataLinkSet",6);
}

int
OpenDDS::DCPS::DataLinkSet::insert_link(const DataLink_rch& link)
{
  DBG_ENTRY_LVL("DataLinkSet","insert_link",6);
  GuardType guard(this->lock_);
  const int retval = OpenDDS::DCPS::bind(map_, link->id(), link);
  VDBG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataLinkSet::insert_link: ")
        ACE_TEXT("added link [%@] id %d %C to map\n"),
        link.in(), link->id(), retval == 0 ? "successfully" : "NOT"));
  return retval;
}

void
OpenDDS::DCPS::DataLinkSet::remove_link(const DataLink_rch& link)
{
  DBG_ENTRY_LVL("DataLinkSet", "remove_link", 6);
  GuardType guard1(this->lock_);
  const int retval = unbind(map_, link->id());
  VDBG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataLinkSet::remove_link: ")
        ACE_TEXT("link [%@] id %d %Cfound in map.\n"),
        link.in(), link->id(), retval == 0 ? "" : "NOT "));
}

OpenDDS::DCPS::DataLinkSet_rch
OpenDDS::DCPS::DataLinkSet::select_links(const GUID_t* remoteIds,
                                         const CORBA::ULong num_targets)
{
  DBG_ENTRY_LVL("DataLinkSet","select_links",6);

  DataLinkSet_rch selected_links ( make_rch<DataLinkSet>() );
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

  return selected_links;
}

bool
OpenDDS::DCPS::DataLinkSet::empty()
{
  GuardType guard(this->lock_);

  return map_.empty();
}

void OpenDDS::DCPS::DataLinkSet::terminate_send_if_suspended()
{
  MapType map_copy;
  {
    GuardType guard(lock_);
    map_copy = map_;
  }
  for (MapType::iterator itr = map_copy.begin();
      itr != map_copy.end(); ++itr) {
        itr->second->terminate_send_if_suspended();
  }
}

OpenDDS::DCPS::SequenceNumber OpenDDS::DCPS::DataLinkSet::cur_cumulative_ack(const GUID_t& writer_id,
                                                                             const GUID_t& reader_id) const
{
  SequenceNumber sn = SequenceNumber::ZERO();

  GuardType guard(this->lock_);
  // Note: Only one DataLink will handle the writer/reader pair.  The other DataLinks will return 0.
  for (MapType::const_iterator pos = map_.begin(), limit = map_.end(); pos != limit; ++pos) {
    sn = std::max(sn, pos->second->cur_cumulative_ack(writer_id, reader_id));
  }

  return sn;
}

bool OpenDDS::DCPS::DataLinkSet::is_leading(const GUID_t& writer_id,
                                            const GUID_t& reader_id) const
{
  GuardType guard(this->lock_);
  for (MapType::const_iterator pos = map_.begin(), limit = map_.end(); pos != limit; ++pos) {
    // Note: Only one DataLink will handle the writer/reader pair.  The other DataLinks will return false.
    if (pos->second->is_leading(writer_id, reader_id)) {
      return true;
    }
  }

  return false;
}
