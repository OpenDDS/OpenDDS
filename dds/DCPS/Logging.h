/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LOGGING_H
#define OPENDDS_DCPS_LOGGING_H

#include "dcps_export.h"

#include "TimeTypes.h"

#include "GuidUtils.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

extern void OpenDDS_Dcps_Export log_progress(const char* activity,
                                             const GUID_t& local,
                                             const GUID_t& remote,
                                             const MonotonicTime_t& start_time,
                                             const GUID_t& reference = GUID_UNKNOWN);

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_LOGGING_H */
