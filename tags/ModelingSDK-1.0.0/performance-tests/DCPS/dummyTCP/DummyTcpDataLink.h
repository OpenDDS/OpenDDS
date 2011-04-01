// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DUMMYTCPDATALINK_H
#define OPENDDS_DCPS_DUMMYTCPDATALINK_H

#include "DummyTcpConnection_rch.h"
#include "DummyTcpTransport_rch.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "ace/INET_Addr.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class TransportSendStrategy;
    class TransportReceiveStrategy;


    class DummyTcpDataLink : public DataLink
    {
      public:

        DummyTcpDataLink(const ACE_INET_Addr& remote_address,
                          DummyTcpTransport*  transport_impl);
        virtual ~DummyTcpDataLink();

        /// Accessor for the remote address.
        const ACE_INET_Addr& remote_address() const;

        /// Called when an established connection object is available
        /// for this DummyTcpDataLink.  Called by the DummyTcpTransport's
        /// connect_datalink() method.
        int connect(DummyTcpConnection*      connection,
                    TransportSendStrategy*    send_strategy,
                    TransportReceiveStrategy* receive_strategy);

        int reconnect (DummyTcpConnection* connection);

        DummyTcpConnection_rch get_connection ();
        DummyTcpTransport_rch get_transport_impl ();

        virtual void pre_stop_i();

        /// Called on subscriber side to send the fully association
        /// message to the publisher.
        virtual void fully_associated ();

      protected:

        /// Called when the DataLink is self-releasing because all of its
        /// reservations have been released, or when the TransportImpl is
        /// handling a shutdown() call.
        virtual void stop_i();


      private:

        void send_graceful_disconnect_message ();

        ACE_INET_Addr           remote_address_;
        DummyTcpConnection_rch connection_;
        DummyTcpTransport_rch  transport_;
        bool graceful_disconnect_sent_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "DummyTcpDataLink.inl"
#endif /* __ACE_INLINE__ */


#endif  /* OPENDDS_DCPS_DUMMYTCPDATALINK_H */
