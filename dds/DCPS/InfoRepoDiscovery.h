/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_INFOREPODISCOVERY_H
#define OPENDDS_DDS_DCPS_INFOREPODISCOVERY_H

#include "Discovery.h"
#include "dds/DdsDcpsInfoC.h"
#include "dds/DCPS/transport/framework/TransportConfig_rch.h"

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class FailoverListener;

/**
 * @class InfoRepoDiscovery
 *
 * @brief Discovery Strategy class that implements InfoRepo discovery
 *
 * This class implements the Discovery interface for InfoRepo-based
 * discovery.
 *
 */
class OpenDDS_Dcps_Export InfoRepoDiscovery : public Discovery {
public:
  InfoRepoDiscovery(const RepoKey& key, const std::string& ior);
  virtual ~InfoRepoDiscovery();

  virtual std::string get_stringified_dcps_info_ior();
  virtual DCPSInfo_ptr get_dcps_info();

  int bit_transport_port() const { return bit_transport_port_; }
  void bit_transport_port(int port) {
    bit_transport_port_ = port;
    use_local_bit_config_ = true;
  }

  std::string bit_transport_ip() const { return bit_transport_ip_; }
  void bit_transport_ip(const std::string& ip) {
    bit_transport_ip_ = ip;
    use_local_bit_config_ = true;
  }

  virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant);

  virtual RepoId bit_key_to_repo_id(DomainParticipantImpl* participant,
                                    const char* bit_topic_name,
                                    const DDS::BuiltinTopicKey_t& key) const;
private:
  TransportConfig_rch bit_config();

  std::string    ior_;
  DCPSInfo_var   info_;

  /// The builtin topic transport address.
  std::string bit_transport_ip_;

  /// The builtin topic transport port number.
  int bit_transport_port_;

  bool use_local_bit_config_;
  TransportConfig_rch bit_config_;

  /// Listener to initiate failover with.
  FailoverListener*    failoverListener_;
};

typedef RcHandle<InfoRepoDiscovery> InfoRepoDiscovery_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_INFOREPODISCOVERY_H  */
