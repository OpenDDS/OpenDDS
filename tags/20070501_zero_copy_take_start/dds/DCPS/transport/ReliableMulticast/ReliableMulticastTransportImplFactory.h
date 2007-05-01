// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTFACTORY_H
#define TAO_DCPS_RELIABLEMULTICASTFACTORY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"

namespace TAO
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

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTFACTORY_H */
