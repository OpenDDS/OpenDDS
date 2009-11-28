/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastSocket.h"

#if !defined (__ACE_INLINE__)
#include "SimpleMcastSocket.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleMcastSocket::~SimpleMcastSocket()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","~SimpleMcastSocket",6);
}
