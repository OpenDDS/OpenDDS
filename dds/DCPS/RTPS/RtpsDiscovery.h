/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_RTPS_RTPSDISCOVERY_H

#include "dds/DCPS/Discovery.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DdsDcpsInfoC.h"
#include "rtps_export.h"
#include "ace/Configuration.h"

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace RTPS {

class RtpsInfo;

/**
 * @class RtpsDiscovery
 *
 * @brief Discovery Strategy class that implements RTPS discovery
 *
 * This class implements the Discovery interface for Rtps-based
 * discovery.
 *
 */
class OpenDDS_Rtps_Export RtpsDiscovery : public OpenDDS::DCPS::Discovery {
public:
  explicit RtpsDiscovery(const RepoKey& key);
  ~RtpsDiscovery();

  virtual OpenDDS::DCPS::DCPSInfo_ptr get_dcps_info();
  virtual DDS::Subscriber_ptr init_bit(DCPS::DomainParticipantImpl* dpi);

private:
  RtpsInfo* servant_;
  OpenDDS::DCPS::DCPSInfo_var info_;

  static int load_rtps_discovery_configuration(ACE_Configuration_Heap& cf);

  class StaticInitializer {
  public:
    StaticInitializer() {
      OpenDDS::DCPS::rtps_discovery_config = RtpsDiscovery::load_rtps_discovery_configuration;
    }
  };
  static StaticInitializer dummy_;
};

typedef OpenDDS::DCPS::RcHandle<RtpsDiscovery> RtpsDiscovery_rch;

} // namespace RTPS
} // namespace OpenDDS

#endif /* OPENDDS_RTPS_RTPSDISCOVERY_H  */
