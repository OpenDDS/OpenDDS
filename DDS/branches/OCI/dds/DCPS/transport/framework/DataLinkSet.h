// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DATALINKSET_H
#define TAO_DCPS_DATALINKSET_H

#include  "dds/DCPS/RcObject_T.h"
#include  "DataLink_rch.h"
#include  "TransportDefs.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class TransportSendListener;
    class DataLinkSetMap;
    struct DataSampleListElement;


    class DataLinkSet : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        DataLinkSet();
        virtual ~DataLinkSet();

        // Returns 0 for success, -1 for failure, and 1 for failure due
        // to duplicate entry (link is already a member of the set).
        int insert_link(DataLink* link);

        /// This method is called to remove a set of DataLinks from this set
        /// (ie, set subtraction: this set minus released_set).
        /// Returns the num elems in the set after attempting the operation.
        ssize_t remove_links(DataLinkSet* released_set);

        /// Remove all reservations involving the remote_id from each
        /// DataLink in this set.  The supplied 'released' map will be
        /// updated with all of the local_id to DataLink reservations that
        /// were made invalid as a result of the release operation.
        void release_reservations(RepoId          remote_id,
                                  DataLinkSetMap& released_locals);

        /// Send to each DataLink in the set.
        void send(DataSampleListElement* sample);

        /// Send control message to each DataLink in the set.
        SendControlStatus send_control(RepoId                 pub_id,
                                       TransportSendListener* listener,
                                       ACE_Message_Block*     msg);

        int remove_sample(const DataSampleListElement* sample);

        int remove_all_control_msgs(RepoId pub_id);

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


      private:

        typedef ACE_Hash_Map_Manager_Ex<DataLinkIdType,
                                        DataLink_rch,
                                        ACE_Hash<DataLinkIdType>,
                                        ACE_Equal_To<DataLinkIdType>,
                                        ACE_Null_Mutex>               MapType;

        MapType  map_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "DataLinkSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_DATALINKSET_H */
