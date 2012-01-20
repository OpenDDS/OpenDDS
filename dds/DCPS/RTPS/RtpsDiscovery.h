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
#include <vector>

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

  virtual DCPS::RepoId bit_key_to_repo_id(DCPS::DomainParticipantImpl* part,
                                          const char* bit_topic_name,
                                          const DDS::BuiltinTopicKey_t& key) const;

  // configuration parameters:

  ACE_Time_Value resend_period() const { return resend_period_; }
  void resend_period(const ACE_Time_Value& period) {
    resend_period_ = period;
  }

  u_short pb() const { return pb_; }
  void pb(u_short port_base) {
    pb_ = port_base;
  }

  u_short dg() const { return dg_; }
  void dg(u_short domain_gain) {
    dg_ = domain_gain;
  }

  u_short pg() const { return pg_; }
  void pg(u_short participant_gain) {
    pg_ = participant_gain;
  }

  u_short d0() const { return d0_; }
  void d0(u_short offset_zero) {
    d0_ = offset_zero;
  }

  u_short d1() const { return d1_; }
  void d1(u_short offset_one) {
    d1_ = offset_one;
  }

  u_short dx() const { return dx_; }
  void dx(u_short offset_two) {
    dx_ = offset_two;
  }

  bool sedp_multicast() const { return sedp_multicast_; }
  void sedp_multicast(bool sm) {
    sedp_multicast_ = sm;
  }

  typedef std::vector<std::string> AddrVec;
  const AddrVec& spdp_send_addrs() const { return spdp_send_addrs_; }
  AddrVec& spdp_send_addrs() { return spdp_send_addrs_; }

private:
  RtpsInfo* servant_;
  OpenDDS::DCPS::DCPSInfo_var info_;

  ACE_Time_Value resend_period_;
  u_short pb_, dg_, pg_, d0_, d1_, dx_;
  bool sedp_multicast_;
  AddrVec spdp_send_addrs_;

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
