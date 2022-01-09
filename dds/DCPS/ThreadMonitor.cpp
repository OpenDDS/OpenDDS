/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "ThreadMonitor.h"
#include "ThreadStatusManager.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadMonitor::UpdateMode IMPLICIT_IDLE = {true, true, true};
ThreadMonitor::UpdateMode EXPLICIT_IDLE = {false, true, true};
ThreadMonitor::UpdateMode IMPLICIT_BUSY = {true, false, true};
ThreadMonitor::UpdateMode EXPLICIT_BUSY = {false, false, true};
ThreadMonitor::UpdateMode INITIAL_MODE = {true, false, false};
ThreadMonitor::UpdateMode FINAL_MODE = {false, false, false};
ThreadMonitor *ThreadMonitor::installed_monitor_ = 0;

ThreadMonitor::~ThreadMonitor()
{
}

void ThreadMonitor::preset(ThreadStatusManager*, const char*)
{
}

size_t ThreadMonitor::thread_count()
{
  return 0;
}

void ThreadMonitor::summarize()
{
}

void ThreadMonitor::report()
{
}

void ThreadMonitor::update(UpdateMode, const char*)
{
}

double ThreadMonitor::get_utilization(const char*) const
{
  return 0.0;
}

ThreadMonitor::GreenLight::GreenLight(const char* alias, bool initial)
: is_initial_(initial)
{
  if (installed_monitor_) {
    installed_monitor_->update(is_initial_ ? INITIAL_MODE : EXPLICIT_BUSY, alias);
  }
}

ThreadMonitor::GreenLight::~GreenLight()
{
  if (installed_monitor_ && !is_initial_) {
    installed_monitor_->update(IMPLICIT_IDLE);
  }
}


ThreadMonitor::RedLight::RedLight(const char* alias, bool final)
: is_final_(final)
{
  if (installed_monitor_) {
    installed_monitor_->update(is_final_ ? FINAL_MODE : EXPLICIT_IDLE, alias);
  }
}

ThreadMonitor::RedLight::~RedLight()
{
  if (installed_monitor_ && !is_final_) {
    installed_monitor_->update(IMPLICIT_BUSY);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
