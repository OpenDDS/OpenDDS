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

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace OpenDDS {
namespace DCPS {

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
  InfoRepoDiscovery(RepoKey      key,
                    std::string  ior);
  virtual std::string get_stringified_dcps_info_ior();
  virtual DCPSInfo_ptr get_dcps_info();

  /**
   * Accessors for @c bit_transport_port_.
   *
   * The accessor is used for client application to configure
   * the local transport listening port number.
   *
   * @note The default port is INVALID. The user needs call
   *       this function to setup the desired port number.
   */
  //@{
  int bit_transport_port() const { return bit_transport_port_; }
  void bit_transport_port(int port) { bit_transport_port_ = port; }
  //@}

  std::string bit_transport_ip() const { return bit_transport_ip_; }
  void bit_transport_ip(const std::string& ip) { bit_transport_ip_ = ip; }

private:
  std::string    ior_;
  DCPSInfo_var   info_;

  // TODO: BIT transport information is now stored her on a per-repository
  // basis but is still not used to configure the BIT transports (this is
  // done using the global settings).

  /// The builtin topic transport address.
  std::string bit_transport_ip_;

  /// The builtin topic transport port number.
  int bit_transport_port_;
};

typedef RcHandle<InfoRepoDiscovery> InfoRepoDiscovery_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_INFOREPODISCOVERY_H  */
