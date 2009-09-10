/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportConfiguration.h"
#include "ace/INET_Addr.h"

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleUnreliableDgramConfiguration
  : public TransportConfiguration {
public:

  SimpleUnreliableDgramConfiguration();
  virtual ~SimpleUnreliableDgramConfiguration();

  /// Describes the local endpoint.
  ACE_TString local_address_str_;

  /// The address string used to provide to DCPSInfoRepo.
  /// This string is either from configuration file or default
  /// to hostname:port. The hostname is fully qualified hostname
  /// and the port is randomly picked by os.
  ACE_INET_Addr local_address_;

  /// Maximum period (in milliseconds) of not being able to send queued
  /// messages. If there are samples queued and no output for longer
  /// than this period then the socket will be closed and on_*_lost()
  /// callbacks will be called. If the value is zero, the default, then
  /// this check will not be made.
  int max_output_pause_period_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramConfiguration.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H */
