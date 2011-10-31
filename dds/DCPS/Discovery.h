/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_DISCOVERY_H
#define OPENDDS_DDS_DCPS_DISCOVERY_H

#include "dds/DdsDcpsInfoC.h"
#include "RcObject_T.h"
#include "RcHandle_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace OpenDDS {
namespace DCPS {

/**
 * @class Discovery
 *
 * @brief Discovery Strategy interface class
 *
 * This class is an abstract class that acts as an interface for both
 * InfoRepo-based discovery and RTPS Discovery.
 *
 */
class OpenDDS_Dcps_Export Discovery : public RcObject<ACE_SYNCH_MUTEX> {
public:
  /// Key type for storing discovery objects.
  /// Probably should just be Discovery::Key
  typedef std::string RepoKey;

  Discovery(RepoKey key) : key_(key) { }

  /// Key value for the default repository IOR.
  static const std::string DEFAULT_REPO;
  static const std::string DEFAULT_RTPS;

  virtual std::string get_stringified_dcps_info_ior();
  virtual DCPSInfo_ptr get_dcps_info()=0;

  // Need one or more virtual functions to abstract away Built-In Topic
  // variation between InfoRepo and RTPS Discovery.

  RepoKey key() const { return this->key_; }

private:
  RepoKey        key_;

};

typedef RcHandle<Discovery> Discovery_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DISCOVERY_H  */
