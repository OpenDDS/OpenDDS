// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H
#define TAO_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastTransportReceiveStrategy
      : public TransportReceiveStrategy
    {
    public:
      virtual ~ReliableMulticastTransportReceiveStrategy();

    protected:
      virtual ssize_t receive_bytes(
        iovec iov[],
        int n,
        ACE_INET_Addr& remote_address
        );

      virtual void deliver_sample(
        ReceivedDataSample& sample,
        const ACE_INET_Addr& remote_address
        );

      virtual int start_i();

      virtual void stop_i();
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H */
