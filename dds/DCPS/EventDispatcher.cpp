/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "EventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

EventBase::~EventBase()
{
}

void EventBase::handle_error()
{
}

void EventBase::handle_cancel()
{
}

void EventBase::operator()()
{
  try {
    handle_event();
  } catch (...) {
    handle_error();
  }
  /// In order to avoid extra allocations for containers of RcHandle<EventBase>
  /// implementations of EventDispatcher will increase the reference count of
  /// all queued EventBase objects, and decrement them when they are dispatched
  /// or canceled.
  /// Since the EventDispatcher does not directly hold copies of the events,
  /// decrementing the reference count after a normal dispatch currently needs
  /// to happen here in the call to EventBase::operator()() after handle_event.
  this->_remove_ref();
}

EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
