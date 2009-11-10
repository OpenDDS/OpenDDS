/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "Packet.h"

#if !defined (__ACE_INLINE__)
#include "Packet.inl"
#endif /* __ACE_INLINE__ */

bool
OpenDDS::DCPS::ReliableMulticast::detail::Packet::operator<(
  const Packet& rhs) const
{
  return (type_ == rhs.type_) ? (id_ < rhs.id_) : (type_ < rhs.type_);
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::Packet::operator==(
  const Packet& rhs) const
{
  bool ok =
    id_ == rhs.id_ &&
    type_ == rhs.type_;

  if (type_ == NACK) {
    ok &=
      (nack_begin_ == rhs.nack_begin_) &&
      (nack_end_ == rhs.nack_end_);

  } else if (
    type_ == DATA_INTERMEDIATE ||
    type_ == DATA_END_OF_MESSAGE) {
    ok &= payload_ == rhs.payload_;
  }

  return ok;
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::Packet::operator!=(
  const Packet& rhs) const
{
  return !(*this == rhs);
}
