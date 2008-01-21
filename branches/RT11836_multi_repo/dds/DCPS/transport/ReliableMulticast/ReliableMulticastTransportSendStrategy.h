// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H
#define OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "ace/Auto_Ptr.h"

namespace OpenDDS
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        class ReactivePacketSender;

      } /* namespace detail */

    } /* namespace ReliableMulticast */

    class ReliableMulticastTransportConfiguration;
    class ReliableMulticastThreadSynchResource;

    class ReliableMulticast_Export ReliableMulticastTransportSendStrategy
      : public TransportSendStrategy
    {
    public:
      // We do not own synch_resource!
      ReliableMulticastTransportSendStrategy(
        OpenDDS::DCPS::ReliableMulticastTransportConfiguration& configuration,
        OpenDDS::DCPS::ReliableMulticastThreadSynchResource* synch_resource
        );
      virtual ~ReliableMulticastTransportSendStrategy();

      void configure(
        ACE_Reactor* reactor,
        const ACE_INET_Addr& local_address,
        const ACE_INET_Addr& multicast_group_address,
        size_t sender_history_size
        );

      void teardown();

    protected:
      virtual void stop_i();

      virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);

      virtual ACE_HANDLE get_handle();

      virtual ssize_t send_bytes_i(const iovec iov[], int n);

    private:
      ACE_Auto_Ptr<OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender> sender_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTSENDSTRATEGY_H */
