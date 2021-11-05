/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ThreadMonitor.h"

using namespace OpenDDS::DCPS;

Thread_Monitor Thread_Monitor::noop_monitor_;
Thread_Monitor *Thread_Monitor::installed_monitor_ = &Thread_Monitor::noop_monitor_;
