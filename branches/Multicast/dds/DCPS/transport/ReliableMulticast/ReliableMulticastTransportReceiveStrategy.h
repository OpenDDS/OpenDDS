/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H
#define OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "detail/PacketReceiverCallback.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "ace/Auto_Ptr.h"
#include <vector>

namespace OpenDDS {
namespace DCPS {

namespace ReliableMulticast {
namespace detail {

class ReactivePacketReceiver;

} // namespace detail
} // namespace ReliableMulticast

class ReliableMulticastDataLink;

class ReliableMulticast_Export ReliableMulticastTransportReceiveStrategy
  : public TransportReceiveStrategy
  , public OpenDDS::DCPS::ReliableMulticast::detail::PacketReceiverCallback {
public:
  ReliableMulticastTransportReceiveStrategy(
    ReliableMulticastDataLink& data_link);
  virtual ~ReliableMulticastTransportReceiveStrategy();

  void configure(
    ACE_Reactor* reactor,
    const ACE_INET_Addr& multicast_group_address,
    size_t receiver_buffer_size);

  void teardown();

  virtual void received_packets(
    const std::vector<OpenDDS::DCPS::ReliableMulticast::detail::Packet>& packets);

  virtual void reliability_compromised();

protected:
  virtual ssize_t receive_bytes(
    iovec iov[],
    int n,
    ACE_INET_Addr& remote_address);

  virtual void deliver_sample(
    ReceivedDataSample& sample,
    const ACE_INET_Addr& remote_address);

  virtual int start_i();

  virtual void stop_i();

private:
  ReliableMulticastDataLink& data_link_;
  ACE_Auto_Ptr<OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver> receiver_;
  std::vector<OpenDDS::DCPS::ReliableMulticast::detail::Packet> buffered_packets_;
  std::vector<OpenDDS::DCPS::ReliableMulticast::detail::Packet> to_deliver_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTRECEIVESTRATEGY_H */
