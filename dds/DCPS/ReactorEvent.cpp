/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ReactorEvent.h"

#include "ace/Reactor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ReactorEvent::ReactorEvent(ACE_Reactor* reactor, EventBase_rch event)
 : reactor_(reactor)
 , event_(event)
{
}

void ReactorEvent::handle_event()
{
  if (reactor_) {
    reactor_->notify(this);
  }
}

int ReactorEvent::handle_exception(ACE_HANDLE)
{
  event_->handle_event();
  return 0;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
