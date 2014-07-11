/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ASYNC_DEBUG_H
#define OPENDDS_DCPS_ASYNC_DEBUG_H

#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

///This is a TEMPORARY debugging constant to aid in the development
///of asynchronous association on the async_assoc branch.

extern OpenDDS_Dcps_Export unsigned int ASYNC_debug;

/// The proper way to set the DCPS_debug_level.
/// This function allows for possible side-effects of setting the level.
extern void OpenDDS_Dcps_Export set_ASYNC_debug(unsigned int lvl);

} // namespace OpenDDS
} // namespace DCPS

#endif /* OPENDDS_DCPS_ASYNC_DEBUG_H */
