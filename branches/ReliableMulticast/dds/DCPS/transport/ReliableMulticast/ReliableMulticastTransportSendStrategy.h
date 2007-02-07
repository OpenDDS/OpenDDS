// -*- C++ -*-
//
// $Id$

#ifndef TAO_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H
#define TAO_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastTransportSendStrategy
      : public TransportSendStrategy
    {
    public:
      ReliableMulticastTransportSendStrategy();
      virtual ~ReliableMulticastTransportSendStrategy();

    protected:
      virtual void stop_i();

      virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);

      virtual ACE_HANDLE get_handle();

      virtual ssize_t send_bytes_i(const iovec iov[], int n);
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H */
