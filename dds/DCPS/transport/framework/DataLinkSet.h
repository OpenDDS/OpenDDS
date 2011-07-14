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
#include "TransportDefs.h"
#include "TransportSendControlElement.h"

#include "ace/Synch.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class TransportSendListener;
class DataLinkSetMap;
struct DataSampleListElement;

class OpenDDS_Dcps_Export DataLinkSet : public RcObject<ACE_SYNCH_MUTEX> {
public:

  DataLinkSet();
  virtual ~DataLinkSet();

  // ciju: Called with lock held in DataLinkSetMap
  // Returns 0 for success, -1 for failure, and 1 for failure due
  // to duplicate entry (link is already a member of the set).
  int insert_link(DataLink* link);

  // ciju: Called with lock held in DataLinkSetMap
  /// This method is called to remove a set of DataLinks from this set
  /// (ie, set subtraction: this set minus released_set).
  /// Returns the num elems in the set after attempting the operation.
  ssize_t remove_links(DataLinkSet* released_set);

  // ciju: Called with lock held in DataLinkSetMap
  /// Remove all reservations involving the remote_id from each
  /// DataLink in this set.  The supplied 'released' map will be
  /// updated with all of the local_id to DataLink reservations that
  /// were made invalid as a result of the release operation.
  //void release_reservations(RepoId          remote_id,
  //                          DataLinkSetMap& released_locals);

  /// Send to each DataLink in the set.
  void send(DataSampleListElement* sample);

  /// Send control message to each DataLink in the set.
  SendControlStatus send_control(RepoId                 pub_id,
                                 TransportSendListener* listener,
                                 ACE_Message_Block*     msg);

  void send_response(RepoId sub_id, ACE_Message_Block* response);

  int remove_sample(const DataSampleListElement* sample, bool dropped_by_transport);

  int remove_all_msgs(RepoId pub_id);

  /// This will do several things, including adding to the membership
  /// of the send_links_ set.  Any DataLinks added to the send_links_
  /// set will be also told about the send_start() event.  Those
  /// DataLinks (in the pub_links set) that are already in the
  /// send_links_ set will not be told about the send_start() event
  /// since they heard about it when they were inserted into the
  /// send_links_ set.
  void send_start(DataLinkSet* link_set);

  /// This will inform each DataLink in the set about the send_stop()
  /// event.  It will then clear the send_links_ set.
  void send_stop();

  DataLinkSet* select_links(const RepoId* remoteIds,
                            const CORBA::ULong num_targets);

  /// Find the datalink with association of remote/local ids. If the remote/local
  /// pair is the only association in the link then the link will be removed
  /// from the map.
  DataLink* find_link(const RepoId remoteId,
                      const RepoId localId,
                      const bool   pub_side);

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
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_DATALINKSET_H */
