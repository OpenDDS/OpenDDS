/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATALINKSETMAP_H
#define OPENDDS_DCPS_DATALINKSETMAP_H

#include "dds/DCPS/dcps_export.h"
#include "DataLinkSet.h"
#include "DataLinkSet_rch.h"
#include "TransportDefs.h"
#include "dds/DCPS/Definitions.h"
#include "ace/Synch.h"
#include <map>

namespace OpenDDS {
namespace DCPS {

class DataLink;

class OpenDDS_Dcps_Export DataLinkSetMap {
public:

  DataLinkSetMap();
  virtual ~DataLinkSetMap();

  /// Caller responsible for reference (count) returned.
  /// Will return nil (0) for failure.
  DataLinkSet* find_or_create_set(RepoId id);

  /// Caller responsible for reference (count) returned.
  /// Will return nil (0) for failure.
  DataLinkSet* find_set(RepoId id);

  DataLinkSet* find_set(RepoId id,
                        const RepoId* remoteIds,
                        const CORBA::ULong num_targets);

  // ciju: Called with TransportImpl Reservation lock held
  /// This method will do the find_or_create_set(id), followed by
  /// an insert() call on the DataLinkSet (the one that was
  /// found or created for us).  A -1 is returned if there are
  /// any problems.  A 0 is returned to denote success.  And
  /// a return code of 1 indicates that the link is already a
  /// member of the DataLinkSet associated with the key RepoId.
  /// REMEMBER: This really means find_or_create_set_then_insert_link()
  int insert_link(RepoId id, DataLink* link);

  // ciju: Called with TransportImpl Reservation lock held
  /// Used by the TransportInterface when this map is regarded as
  /// the "remote map".
  ///
  /// For each remote_id in the array of remote_ids, this method
  /// will cause the remote_id/local_id DataLink be removed from
  /// remote_id's DataLinkSet if the remote_id/local_id is the
  /// last association in the DataLink, followed by informing the
  /// remote_id/local_id DataLink to release the remote_id/local_id.
  /// The DataLink will update the released_locals as it successfully
  /// handles its release_reservation() requests.
  void release_reservations(ssize_t         num_remote_ids,
                            const RepoId*   remote_ids,
                            const RepoId    local_id,
                            DataLinkSetMap& released_locals,
                            const bool pub_side);

  // ciju: Called with TransportImpl Reservation lock held
  /// Called when the TransportInterface is detaching from the
  /// TransportImpl (as opposed to the other way around when the
  /// TransportImpl is detaching from the TransportInterface).
  void release_all_reservations();

  // ciju: Called with TransportImpl Reservation lock held
  /// Used by the TransportInterface when this map is regarded as
  /// the "local map".
  ///
  /// The supplied released_locals contains, for each RepoId key,
  /// the set of DataLinks that should be removed from our map_.
  /// These are removed due to a release_reservations call on our
  /// "reverse" map in the TransportInterface.
  void remove_released(const DataLinkSetMap& released_locals);

  /// Make the map_ empty.
  void clear();

  /// Diagnostic aid.
  void dump(); // Not const to allow use of map_lock_

private:

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  typedef std::map <RepoId, DataLinkSet_rch, GUID_tKeyLessThan>      MapType;

  LockType map_lock_; // This lock is explicitly for this->map_ protection
  MapType  map_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DATALINKSETMAP_H */
