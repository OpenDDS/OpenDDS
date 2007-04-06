// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H
#define TAO_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportConfiguration.h"
#include "ace/INET_Addr.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastTransportConfiguration
      : public TransportConfiguration
    {
    public:
      ReliableMulticastTransportConfiguration();
      virtual ~ReliableMulticastTransportConfiguration();

      virtual int load(
        const TransportIdType& id,
        ACE_Configuration_Heap& config
        );

      ACE_INET_Addr local_address_;
      ACE_INET_Addr multicast_group_address_;
      bool receiver_;
      size_t sender_history_size_;
      size_t receiver_buffer_size_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportConfiguration.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H */
