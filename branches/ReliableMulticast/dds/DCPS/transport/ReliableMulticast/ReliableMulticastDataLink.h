// -*- C++ -*-
//
// $Id$

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
        const ACE_INET_Addr& multicast_group_address,
        TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
        );
      virtual ~ReliableMulticastDataLink();

      bool connect(bool is_publisher);

    protected:
      virtual void stop_i();

    private:
      TAO::DCPS::TransportReactorTask_rch reactor_task_;
      TAO::DCPS::ReliableMulticastTransportImpl_rch transport_impl_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTDATALINK_H */
