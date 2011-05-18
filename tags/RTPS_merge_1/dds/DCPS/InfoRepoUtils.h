/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INFOREPOUTILS_H
#define OPENDDS_DCPS_INFOREPOUTILS_H

#include "dds/DdsDcpsInfoC.h"

namespace OpenDDS {
namespace DCPS {

  namespace InfoRepoUtils
  {
    /// Get and narrow the InfoRepo from an ior.
    /// Accepts "host:port" as a valid InfoRepo ior.
    /// Returns the InfoRepo if successful, nil if not
    DCPSInfo_ptr get_repo(const char* ior, CORBA::ORB_ptr orb);
  } // InfoRepoUtils

} // DCPS
} // OpenDDS

#endif /* OPENDDS_DCPS_INFOREPOUTILS_H */
