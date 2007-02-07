// -*- C++ -*-
//
// $Id$

#ifndef TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H
#define TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"

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
      virtual DataLink* find_or_create_datalink(
        const TransportInterfaceInfo& remote_info,
        int connect_as_publisher
        );

      virtual int configure_i(TransportConfiguration* config);

      virtual void shutdown_i();

      virtual int connection_info_i(TransportInterfaceInfo& local_info) const;

      virtual void release_datalink_i(DataLink* link);

    private:
      ReliableMulticastTransportConfiguration* configuration_;
      // JSP: Add transport configuration storage
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImpl.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTRANSPORT_H */
