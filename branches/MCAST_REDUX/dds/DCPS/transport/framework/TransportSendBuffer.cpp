/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TransportSendBuffer.h"

#ifndef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportSendBuffer::TransportSendBuffer(size_t capacity)
  : capacity_(capacity)
{
}

TransportSendBuffer::~TransportSendBuffer()
{
  release_all();
}

void
TransportSendBuffer::insert(SequenceNumber sequence, const buffer_type& value)
{
}

void
TransportSendBuffer::release_all()
{
}

void
TransportSendBuffer::release(const buffer_type& value)
{
}

void
TransportSendBuffer::retain(RepoId pub_id)
{
}

bool
TransportSendBuffer::resend(const range_type& range, range_type& actual)
{
  return false;
}

bool
TransportSendBuffer::resend(SequenceNumber sequence)
{
  return false;
}

} // namespace DCPS
} // namespace OpenDDS
