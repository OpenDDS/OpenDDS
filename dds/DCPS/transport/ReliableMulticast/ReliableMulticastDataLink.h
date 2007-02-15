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
#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "ace/Auto_Ptr.h"

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReactivePacketReceiver;
        class ReactivePacketSender;

      } /* namespace detail */

    } /* namespace ReliableMulticast */

    class ReliableMulticast_Export ReliableMulticastDataLink
      : public DataLink
    {
    public:
      ReliableMulticastDataLink(
        TransportReactorTask_rch& reactor_task,
        const ACE_INET_Addr& multicast_group_address,
        TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
        );
      virtual ~ReliableMulticastDataLink();

      bool connect(bool is_publisher);

    protected:
      virtual void stop_i();

    private:
      ACE_INET_Addr multicast_group_address_;
      bool is_publisher_;
      TAO::DCPS::TransportReactorTask_rch reactor_task_;
      TAO::DCPS::ReliableMulticastTransportImpl_rch transport_impl_;
      ACE_Auto_Ptr<TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver> receiver_;
      ACE_Auto_Ptr<TAO::DCPS::ReliableMulticast::detail::ReactivePacketSender> sender_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTDATALINK_H */
