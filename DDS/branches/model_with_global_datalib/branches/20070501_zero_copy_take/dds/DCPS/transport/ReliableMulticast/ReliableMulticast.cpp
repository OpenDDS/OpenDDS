// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticast.h"
#include "ReliableMulticastLoader.h"
#include "ace/Dynamic_Service.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticast.inl"
#endif /* __ACE_INLINE__ */

TAO_DCPS_ReliableMulticast_Initializer::TAO_DCPS_ReliableMulticast_Initializer()
{
  ACE_Service_Config::process_directive (ace_svc_desc_TAO_DCPS_ReliableMulticastLoader);
}
