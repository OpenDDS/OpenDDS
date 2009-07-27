// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_RELIABLEMULTICAST_INITIALIZER_H
#define OPENDDS_DCPS_RELIABLEMULTICAST_INITIALIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

class ReliableMulticast_Export OPENDDS_DCPS_ReliableMulticast_Initializer
{
public:
  OPENDDS_DCPS_ReliableMulticast_Initializer();
};

#if defined (__ACE_INLINE__)
#include "ReliableMulticast.inl"
#endif /* __ACE_INLINE__ */

static OPENDDS_DCPS_ReliableMulticast_Initializer OPENDDS_DCPS_ReliableMulticast_initializer;

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICAST_INITIALIZER_H */
