// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_RECEIVELISTENERSET_H
#define TAO_DCPS_RECEIVELISTENERSET_H

#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class TransportReceiveListener;
    class ReceivedDataSample;


    class TAO_DdsDcps_Export ReceiveListenerSet :
                                         public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        typedef ACE_Hash_Map_Manager_Ex<RepoId,
                                        TransportReceiveListener*,
                                        ACE_Hash<RepoId>,
                                        ACE_Equal_To<RepoId>,
                                        ACE_Null_Mutex>        MapType;

        ReceiveListenerSet();
        virtual ~ReceiveListenerSet();

        int insert(RepoId                    subscriber_id,
                   TransportReceiveListener* listener);
        int remove(RepoId subscriber_id);

        ssize_t size() const;

        void data_received(const ReceivedDataSample& sample);

        /// Give access to the underlying map for iteration purposes.
        MapType& map();
        const MapType& map() const;


      private:

        MapType  map_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_RECEIVELISTENERSET_H */
