// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTFACTORY_H
#define OPENDDS_DCPS_RELIABLEMULTICASTFACTORY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"

namespace OpenDDS
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastTransportImplFactory
      : public TransportImplFactory
    {
    public:
      ReliableMulticastTransportImplFactory();
      virtual ~ReliableMulticastTransportImplFactory();

      virtual int requires_reactor() const;

    protected:
      virtual TransportImpl* create();
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTFACTORY_H */
