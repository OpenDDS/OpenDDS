// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICAST_INITIALIZER_H
#define TAO_DCPS_RELIABLEMULTICAST_INITIALIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

class ReliableMulticast_Export TAO_DCPS_ReliableMulticast_Initializer
{
public:
  TAO_DCPS_ReliableMulticast_Initializer();
};

#if defined (__ACE_INLINE__)
#include "ReliableMulticast.inl"
#endif /* __ACE_INLINE__ */

static TAO_DCPS_ReliableMulticast_Initializer TAO_DCPS_ReliableMulticast_initializer;

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICAST_INITIALIZER_H */
