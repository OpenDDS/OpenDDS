// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H
#define TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "ReliableMulticastRcHandles.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include <map>

namespace TAO
{

  namespace DCPS
  {
    class ReliableMulticastTransportConfiguration;

    class ReliableMulticast_Export ReliableMulticastTransportImpl
      : public TransportImpl
    {
    public:
      ReliableMulticastTransportImpl();
      virtual ~ReliableMulticastTransportImpl();

    protected:
      virtual TAO::DCPS::DataLink* find_or_create_datalink(
        const TransportInterfaceInfo& remote_info,
        int connect_as_publisher
        );

      virtual int configure_i(TransportConfiguration* config);

      virtual void shutdown_i();

      virtual int connection_info_i(TransportInterfaceInfo& local_info) const;

      virtual void release_datalink_i(TAO::DCPS::DataLink* link);

      virtual bool acked(RepoId);

    private:
      ReliableMulticastTransportConfiguration* configuration_;
      // JSP: Add transport configuration storage
      typedef std::map<
        ACE_INET_Addr,
        TAO::DCPS::ReliableMulticastDataLink_rch
        > ReliableMulticastDataLinkMap;
      ReliableMulticastDataLinkMap data_links_;
      TransportReactorTask_rch reactor_task_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImpl.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H */
