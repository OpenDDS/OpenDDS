// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTDATALINK_H
#define TAO_DCPS_RELIABLEMULTICASTDATALINK_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "ReliableMulticastRcHandles.h"
#include "ReliableMulticastTransportReceiveStrategy.h"
#include "ReliableMulticastTransportSendStrategy.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastDataLink
      : public DataLink
    {
    public:
      ReliableMulticastDataLink(
        TransportReactorTask_rch& reactor_task,
        ReliableMulticastTransportConfiguration& configuration,
        const ACE_INET_Addr& multicast_group_address,
        TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
        );
      virtual ~ReliableMulticastDataLink();

      bool connect(bool is_publisher);

      TAO::DCPS::ReliableMulticastTransportImpl_rch& get_transport_impl();

    protected:
      virtual void stop_i();

    private:
      ACE_INET_Addr local_address_;
      ACE_INET_Addr multicast_group_address_;
      size_t sender_history_size_;
      size_t receiver_buffer_size_;
      bool is_publisher_;
      TAO::DCPS::TransportReactorTask_rch reactor_task_;
      TAO::DCPS::ReliableMulticastTransportImpl_rch transport_impl_;
      TAO::DCPS::ReliableMulticastTransportReceiveStrategy receive_strategy_;
      TAO::DCPS::ReliableMulticastTransportSendStrategy send_strategy_;
      bool running_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTDATALINK_H */
