/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATALINKSET_H
#define OPENDDS_DCPS_DATALINKSET_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "DataLink_rch.h"
#include "SendResponseListener.h"
#include "TransportDefs.h"
#include "TransportSendControlElement.h"

#include "ace/Synch.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class TransportSendListener;
class DataSampleElement;

class OpenDDS_Dcps_Export DataLinkSet : public RcObject<ACE_SYNCH_MUTEX> {
public:

  DataLinkSet();
  virtual ~DataLinkSet();

  // Returns 0 for success, -1 for failure, and 1 for failure due
  // to duplicate entry (link is already a member of the set).
  int insert_link(DataLink* link);

  void remove_link(const DataLink_rch& link);

  /// Send to each DataLink in the set.
  void send(DataSampleElement* sample);

  /// Send control message to each DataLink in the set.
  SendControlStatus send_control(RepoId                  pub_id,
                                 TransportSendListener*  listener,
                                 const DataSampleHeader& header,
                                 ACE_Message_Block*      msg);

  void send_response(RepoId sub_id,
                     const DataSampleHeader& header,
                     ACE_Message_Block* response);

  bool remove_sample(const DataSampleElement* sample);

  bool remove_all_msgs(RepoId pub_id);

  /// Calls send_start() on the links in link_set and also adds
  /// the links from link_set to *this.
  void send_start(DataLinkSet* link_set);

  /// Calls send_stop() on the links with ID repoId and then
  /// clears the set.
  void send_stop(RepoId repoId);

  DataLinkSet* select_links(const RepoId* remoteIds,
                            const CORBA::ULong num_targets);

  bool empty();

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  typedef std::map<DataLinkIdType, DataLink_rch> MapType;

  //{@
  /// Accessors for external iteration
  LockType& lock() { return lock_; }
  MapType& map() { return map_; }
  //@}

private:

  /// Hash map for DataLinks.
  MapType map_;

  /// Allocator for TransportSendControlElement.
  TransportSendControlElementAllocator send_control_element_allocator_;

  /// This lock will protect critical sections of code that play a
  /// role in the sending of data.
  LockType lock_;

  /// Listener for TransportSendControlElements created in send_response
  SendResponseListener send_response_listener_;

  /// lock and copy map for lock-free access
  void copy_map_to(MapType& target);
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_DATALINKSET_H */
