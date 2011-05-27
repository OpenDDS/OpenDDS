// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTDATALINK_H
#define TAO_DCPS_SIMPLEMCASTDATALINK_H

#include  "dds/DCPS/transport/framework/DataLink.h"
#include  "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class TransportSendStrategy;
    class SimpleMcastTransport;

    class SimpleMcastDataLink : public DataLink
    {
      public:

        SimpleMcastDataLink(const ACE_INET_Addr& remote_address,
                          SimpleMcastTransport*  transport_impl);
        virtual ~SimpleMcastDataLink();

        /// Accessor for the remote address.
        const ACE_INET_Addr& remote_address() const;

        int connect(TransportSendStrategy* send_strategy);


      protected:

        /// Called when the DataLink is self-releasing because all of its
        /// reservations have been released, or when the TransportImpl is
        /// handling a shutdown() call.
        virtual void stop_i();


      private:

        ACE_INET_Addr remote_address_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastDataLink.inl"
#endif /* __ACE_INLINE__ */

#endif
