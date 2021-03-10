/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LOGGING_H
#define OPENDDS_DCPS_LOGGING_H

#include "dcps_export.h"

#include "TimeDuration.h"

#include <dds/DdsDcpsGuidC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

extern void OpenDDS_Dcps_Export log_progress(const char* activity,
                                             const GUID_t& local,
                                             const GUID_t& remote,
                                             const TimeDuration& duration,
                                             size_t messages_sent,
                                             size_t messages_received);

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_LOGGING_H */
