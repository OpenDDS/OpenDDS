/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>
#include "ThreadMonitor.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

Thread_Monitor::UpdateMode IMPLICIT_IDLE = {true, true};
Thread_Monitor::UpdateMode EXPLICIT_IDLE = {false, true};
Thread_Monitor::UpdateMode IMPLICIT_BUSY = {true, false};
Thread_Monitor::UpdateMode EXPLICIT_BUSY = {false, false};
Thread_Monitor *Thread_Monitor::installed_monitor_ = 0;

Thread_Monitor::Green_Light::Green_Light(const char* alias)
{
  if (installed_monitor_) {
    installed_monitor_->update(EXPLICIT_BUSY, alias);
  }
}

Thread_Monitor::Green_Light::~Green_Light(void)
{
  if (installed_monitor_) {
    installed_monitor_->update(IMPLICIT_IDLE);
  }
}


Thread_Monitor::Red_Light::Red_Light(const char* alias)
{
  if (installed_monitor_) {
    installed_monitor_->update(EXPLICIT_IDLE, alias);
  }
}

Thread_Monitor::Red_Light::~Red_Light(void)
{
  if (installed_monitor_) {
    installed_monitor_->update(IMPLICIT_BUSY);
  }
}

  } // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
