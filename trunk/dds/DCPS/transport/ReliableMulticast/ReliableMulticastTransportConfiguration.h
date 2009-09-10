/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H
#define OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/transport/framework/TransportConfiguration.h"
#include "ace/INET_Addr.h"

namespace OpenDDS {

namespace DCPS {

class ReliableMulticast_Export ReliableMulticastTransportConfiguration
  : public TransportConfiguration {
public:
  ReliableMulticastTransportConfiguration();
  virtual ~ReliableMulticastTransportConfiguration();

  virtual int load(
    const TransportIdType& id,
    ACE_Configuration_Heap& config);

  ACE_TString   local_address_str_;
  ACE_INET_Addr local_address_;
  ACE_TString   multicast_group_address_str_;
  ACE_INET_Addr multicast_group_address_;
  bool receiver_;
  size_t sender_history_size_;
  size_t receiver_buffer_size_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportConfiguration.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORTCONFIGURATION_H */
