/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_LOGGING_H
#define OPENDDS_DCPS_RTPS_LOGGING_H

#include "rtps_export.h"

#include "RtpsCoreTypeSupportImpl.h"

#include <dds/Versioned_Namespace.h>

#include <dds/DCPS/GuidUtils.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

/// Log a serialized RTPS message.
OpenDDS_Rtps_Export
void log_message(const char* format,
                 const DCPS::GuidPrefix_t& prefix,
                 bool send,
                 const Message& message);

OpenDDS_Rtps_Export
void parse_submessages(Message& message,
                       const ACE_Message_Block& mb);

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RTPS_LOGGING_H */
