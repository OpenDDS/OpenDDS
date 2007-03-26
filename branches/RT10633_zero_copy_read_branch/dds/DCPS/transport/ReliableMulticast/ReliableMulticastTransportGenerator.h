// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTGENERATOR_H
#define TAO_DCPS_RELIABLEMULTICASTGENERATOR_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportGenerator.h"

namespace TAO
{

  namespace DCPS
  {

    //class TransportConfiguration;
    //class TransportImplFactory;

    class ReliableMulticast_Export ReliableMulticastTransportGenerator
      : public TransportGenerator
    {
    public:
      virtual ~ReliableMulticastTransportGenerator();

      virtual TransportImplFactory* new_factory();

      virtual TransportConfiguration* new_configuration(const TransportIdType id);

      virtual void default_transport_ids (TransportIdList & ids);
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportGenerator.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTGENERATOR_H */
