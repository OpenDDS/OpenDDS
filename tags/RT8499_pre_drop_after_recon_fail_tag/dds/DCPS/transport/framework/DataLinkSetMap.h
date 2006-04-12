// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DATALINKSETMAP_H
#define TAO_DCPS_DATALINKSETMAP_H

#include  "dds/DCPS/dcps_export.h"
#include  "DataLinkSet_rch.h"
#include  "TransportDefs.h"
#include  "dds/DCPS/Definitions.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class DataLink;


    class TAO_DdsDcps_Export DataLinkSetMap
    {
      public:

        DataLinkSetMap();
        virtual ~DataLinkSetMap();

        /// Caller responsible for reference (count) returned.
        /// Will return nil (0) for failure.
        DataLinkSet* find_or_create_set(RepoId id);

        /// Caller responsible for reference (count) returned.
        /// Will return nil (0) for failure.
        DataLinkSet* find_set(RepoId id);

        /// This method will do the find_or_create_set(id), followed by
        /// an insert() call on the DataLinkSet (the one that was
        /// found or created for us).  A -1 is returned if there are
        /// any problems.  A 0 is returned to denote success.  And
        /// a return code of 1 indicates that the link is already a
        /// member of the DataLinkSet associated with the key RepoId.
        /// REMEMBER: This really means find_or_create_set_then_insert_link()
        int insert_link(RepoId id, DataLink* link);

        /// Used by the TransportInterface when this map is regarded as
        /// the "remote map".
        ///
        /// For each remote_id in the array of remote_ids, this method
        /// will cause the remote_id's DataLinkSet to be removed from
        /// our map_, followed by informing the removed DataLinkSet
        /// object to release the remote_id from each of the set's DataLinks.
        /// The DataLinks will update the released_locals as it successfully
        /// handles its release_reservation() requests.
        void release_reservations(ssize_t         num_remote_ids,
                                  const RepoId*   remote_ids,
                                  DataLinkSetMap& released_locals);

        /// Called when the TransportInterface is detaching from the
        /// TransportImpl (as opposed to the other way around when the
        /// TransportImpl is detaching from the TransportInterface).
        void release_all_reservations();

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


      private:

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        typedef ACE_Hash_Map_Manager_Ex<RepoId,
                                        DataLinkSet_rch,
                                        ACE_Hash<RepoId>,
                                        ACE_Equal_To<RepoId>,
                                        ACE_Null_Mutex>        MapType;

        LockType lock_;
        MapType  map_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#endif /* TAO_DCPS_DATALINKSETMAP_H */
