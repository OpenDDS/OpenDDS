/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpSynchResource.h"
#include "TcpConnection.h"
#include "TcpSendStrategy.h"
#include "TcpDataLink.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpSynchResource::TcpSynchResource(
  TcpDataLink& link,
  const int& max_output_pause_period_ms)
  : ThreadSynchResource()
  , link_(link)
{
  DBG_ENTRY_LVL("TcpSynchResource","TcpSynchResource",6);

  if (max_output_pause_period_ms >= 0) {
    timeout_ = new TimeDuration(TimeDuration::from_msec(max_output_pause_period_ms));
  }
}

OpenDDS::DCPS::TcpSynchResource::~TcpSynchResource()
{
  DBG_ENTRY_LVL("TcpSynchResource","~TcpSynchResource",6);
}

void
OpenDDS::DCPS::TcpSynchResource::notify_lost_on_backpressure_timeout()
{
  DBG_ENTRY_LVL("TcpSynchResource","notify_lost_on_backpressure_timeout",6);

  TcpConnection_rch connection = link_.get_connection();
  connection->notify_lost_on_backpressure_timeout();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
