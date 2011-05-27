/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEMCASTCONFIGURATION_H
#define OPENDDS_DCPS_SIMPLEMCASTCONFIGURATION_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramConfiguration.h"
#include "ace/INET_Addr.h"

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleMcastConfiguration
  : public SimpleUnreliableDgramConfiguration {
public:

  SimpleMcastConfiguration();
  virtual ~SimpleMcastConfiguration();

  virtual int load(const TransportIdType& id,
                   ACE_Configuration_Heap& cf);

  /// This is the multicast group to send/receive on.
  ACE_INET_Addr multicast_group_address_;

  /// The address string used to provide to DCPSInfoRepo.
  /// This string is either from configuration file or default
  /// to ACE_DEFAULT_MULTICAST_PORT:ACE_DEFAULT_MULTICAST_ADDR
  ACE_TString  multicast_group_address_str_;

  /// Are we a receiver?
  bool receiver_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleMcastConfiguration.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLEMCASTCONFIGURATION_H */
