/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NACKGENERATOR_H
#define OPENDDS_DCPS_NACKGENERATOR_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "Packet.h"
#include <set>
#include <vector>
#include <iterator>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

class ReliableMulticast_Export NackGenerator {
public:
  typedef std::set<
  OpenDDS::DCPS::ReliableMulticast::detail::Packet
  > PacketSet;

  bool cancel(
    OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type id);

  void cancel_all();

  void nack_range(
    OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type begin,
    OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type end);

  void get_nacks(
    std::vector<OpenDDS::DCPS::ReliableMulticast::detail::Packet>& nacks);

private:
  PacketSet::iterator find_nack_containing(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& packet);

  PacketSet::iterator join_nacks(
    PacketSet::iterator first,
    PacketSet::iterator second);

  PacketSet nacks_;
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "NackGenerator.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_NACKGENERATOR_H */
