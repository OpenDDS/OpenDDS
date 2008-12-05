// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTDATALINK_H
#define OPENDDS_DCPS_RELIABLEMULTICASTDATALINK_H

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

namespace OpenDDS
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
        OpenDDS::DCPS::ReliableMulticastTransportImpl& transport_impl,
        CORBA::Long priority
        );
      virtual ~ReliableMulticastDataLink();

      bool connect(bool is_publisher);

      OpenDDS::DCPS::ReliableMulticastTransportImpl_rch& get_transport_impl();

      /// Access the underlying socket.
      /// N.B. This is valid only for the publication end DataLink
      ///      objects.  This transport distinguishes the endpoint role
      ///      of individual DataLink objects.
      ACE_SOCK& socket();

    protected:
      virtual void stop_i();

    private:
      ACE_INET_Addr local_address_;
      ACE_INET_Addr multicast_group_address_;
      size_t sender_history_size_;
      size_t receiver_buffer_size_;
      bool is_publisher_;
      OpenDDS::DCPS::TransportReactorTask_rch reactor_task_;
      OpenDDS::DCPS::ReliableMulticastTransportImpl_rch transport_impl_;
      OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy receive_strategy_;
      OpenDDS::DCPS::ReliableMulticastTransportSendStrategy send_strategy_;
      bool running_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTDATALINK_H */
