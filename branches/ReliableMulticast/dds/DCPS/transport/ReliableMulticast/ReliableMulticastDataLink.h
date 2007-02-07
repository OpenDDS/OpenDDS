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
#include "dds/DCPS/transport/framework/DataLink.h"

namespace TAO
{

  namespace DCPS
  {

    class ReliableMulticast_Export ReliableMulticastDataLink
      : public DataLink
    {
    public:
      ReliableMulticastDataLink();
      virtual ~ReliableMulticastDataLink();

    protected:
      virtual void stop_i();
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTDATALINK_H */
