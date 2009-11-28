/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"
#include "SimpleTcpReconnectTask.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OpenDDS::DCPS::SimpleTcpReconnectTask::SimpleTcpReconnectTask(
  OpenDDS::DCPS::SimpleTcpConnection* connection)
  : connection_(connection)
{
  DBG_ENTRY_LVL("SimpleTcpReconnectTask","SimpleTcpReconnectTask",6);
}

OpenDDS::DCPS::SimpleTcpReconnectTask::~SimpleTcpReconnectTask()
{
  DBG_ENTRY_LVL("SimpleTcpReconnectTask","~SimpleTcpReconnectTask",6);
}

void OpenDDS::DCPS::SimpleTcpReconnectTask::execute(ReconnectOpType& op)
{
  DBG_ENTRY_LVL("SimpleTcpReconnectTask","execute",6);

  if (op == DO_RECONNECT) {
    if (this->connection_->reconnect() == -1) {
      this->connection_->tear_link();
    }

  } else
    ACE_ERROR((LM_ERROR, "(%P|%t)ERROR: SimpleTcpReconnectTask::svc unknown operation %d\n", op));
}
