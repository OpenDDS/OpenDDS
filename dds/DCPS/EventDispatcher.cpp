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
