/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ThreadMonitor.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

Thread_Monitor Thread_Monitor::noop_monitor_;
Thread_Monitor *Thread_Monitor::installed_monitor_ = &Thread_Monitor::noop_monitor_;

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
