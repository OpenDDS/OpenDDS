/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpSocket.h"

#if !defined (__ACE_INLINE__)
#include "SimpleUdpSocket.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleUdpSocket::~SimpleUdpSocket()
{
  DBG_ENTRY_LVL("SimpleUdpSocket","~SimpleUdpSocket",6);
}
