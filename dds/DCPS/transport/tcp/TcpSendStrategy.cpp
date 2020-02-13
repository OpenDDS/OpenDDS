/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpConnection.h"
#include "TcpSendStrategy.h"
#include "TcpTransport.h"
#include "TcpInst.h"
#include "TcpSynchResource.h"
#include "TcpDataLink.h"
#include "dds/DCPS/transport/framework/ThreadSynch.h"
#include "dds/DCPS/transport/framework/ScheduleOutputHandler.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/transport/framework/ReactorSynchStrategy.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpSendStrategy::TcpSendStrategy(
  std::size_t id,
  TcpDataLink& link,
  TcpSynchResource* synch_resource,
  const ReactorTask_rch& task,
  Priority priority)
  : TransportSendStrategy(id, link.impl(),
                          synch_resource, priority,
                          make_rch<ReactorSynchStrategy>(this,task->get_reactor()))
  , link_(link)
  , reactor_task_(task)
{
  DBG_ENTRY_LVL("TcpSendStrategy","TcpSendStrategy",6);

}

OpenDDS::DCPS::TcpSendStrategy::~TcpSendStrategy()
{
  DBG_ENTRY_LVL("TcpSendStrategy","~TcpSendStrategy",6);
}

void
OpenDDS::DCPS::TcpSendStrategy::schedule_output()
{
  DBG_ENTRY_LVL("TcpSendStrategy","schedule_output",6);

  // Notify the reactor to adjust its processing policy according to mode_.
  synch()->work_available();

  if (DCPS_debug_level > 4) {
    const char* action = "";
    if( mode() == MODE_DIRECT) {
      action = "canceling";
    } else if( (mode() == MODE_QUEUE)
            || (mode() == MODE_SUSPEND)) {
      action = "starting";
    }
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpSendStrategy::schedule_output() [%d] - ")
               ACE_TEXT("%C data queueing for handle %d.\n"),
               id(),action,get_handle()));
  }
}

int
OpenDDS::DCPS::TcpSendStrategy::reset(bool reset_mode)
{
  DBG_ENTRY_LVL("TcpSendStrategy","reset",6);
  //For the case of a send_strategy being reused for a new connection (not reconnect)
  //need to reset the state
  if (reset_mode) {
    //Need to make sure that the send mode is set back to MODE_DIRECT in case
    //it was terminated prior to being reused/reset.
    this->clear(MODE_DIRECT);
    //reset graceful_disconnecting_ to initial state
    this->set_graceful_disconnecting(false);
  }
  return 0;
}

ssize_t
OpenDDS::DCPS::TcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("TcpSendStrategy","send_bytes",6);
  return this->non_blocking_send(iov, n, bp);
}

ACE_HANDLE
OpenDDS::DCPS::TcpSendStrategy::get_handle()
{
  TcpConnection_rch connection = link_.get_connection();

  if (!connection)
    return ACE_INVALID_HANDLE;

  return connection->peer().get_handle();
}

ssize_t
OpenDDS::DCPS::TcpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  TcpConnection_rch connection = link_.get_connection();

  if (!connection)
    return -1;
  ssize_t result = connection->peer().sendv(iov, n);
  if (DCPS_debug_level > 4)
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpSendStrategy::send_bytes_i sent %d bytes \n", result));

  return result;
}

void
OpenDDS::DCPS::TcpSendStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpSendStrategy","relink",6);
  TcpConnection_rch connection = link_.get_connection();

  if (connection) {
    connection->relink_from_send(do_suspend);
  }
}

void
OpenDDS::DCPS::TcpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("TcpSendStrategy","stop_i",6);
}

void
OpenDDS::DCPS::TcpSendStrategy::add_delayed_notification(TransportQueueElement* element)
{
  if (!element->is_request_ack()) {
    // only add the notification when we are not sending REQUEST_ACK message
    TransportSendStrategy::add_delayed_notification(element);
  }
}

void
OpenDDS::DCPS::TcpSendStrategy::terminate_send_if_suspended()
{
  DBG_ENTRY_LVL("TcpSendStrategy","terminate_send_if_suspended",6);
  this->clear(MODE_TERMINATED, MODE_SUSPEND);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
