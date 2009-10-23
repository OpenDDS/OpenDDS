/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReliableMulticast.h"
#include "ReliableMulticastLoader.h"
#include "ace/Dynamic_Service.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticast.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_DCPS_ReliableMulticast_Initializer::OPENDDS_DCPS_ReliableMulticast_Initializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_OPENDDS_DCPS_ReliableMulticastLoader);
#if RELIABLEMULTICAST_HAS_DLL == 0
  ACE_Service_Config::process_directive(
    ACE_TEXT("static OPENDDS_DCPS_ReliableMulticastLoader \"\""));
#endif
}
