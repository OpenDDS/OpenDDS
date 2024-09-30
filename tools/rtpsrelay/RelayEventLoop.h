/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RTPSRELAY_RELAY_EVENT_LOOP_H_
#define RTPSRELAY_RELAY_EVENT_LOOP_H_

#include "Config.h"
#include "RelayThreadMonitor.h"

#include <ace/Reactor.h>

namespace RtpsRelay {
namespace RelayEventLoop {

ACE_Reactor_Impl* make_reactor_impl(const Config& config);

int run(const Config& config, ACE_Reactor& reactor, RelayThreadMonitor& monitor);

}
}

#endif
