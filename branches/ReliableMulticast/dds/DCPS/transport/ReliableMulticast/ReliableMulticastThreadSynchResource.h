// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTTHREADSYNCHRESOURCE_H
#define TAO_DCPS_RELIABLEMULTICASTTHREADSYNCHRESOURCE_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastThreadSynchResource
      : public ThreadSynchResource
    {
    public:
      ReliableMulticastThreadSynchResource();
      virtual ~ReliableMulticastThreadSynchResource();

    protected:
      virtual void notify_lost_on_backpressure_timeout();
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastThreadSynchResource.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTTHREADSYNCHRESOURCE_H */
