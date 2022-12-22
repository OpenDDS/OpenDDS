/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TIME_SOURCE_H
#define OPENDDS_DCPS_TIME_SOURCE_H

#include "TimeTypes.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TimeSource {
public:
  virtual ~TimeSource() {}

  virtual MonotonicTimePoint monotonic_time_point_now() const {
    return MonotonicTimePoint::now();
  }

};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TIME_SOURCE_H */
