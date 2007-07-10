// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpReconnectTask.h"
#include "DummyTcpConnection.h"
#include "DummyTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"



OpenDDS::DCPS::DummyTcpReconnectTask::DummyTcpReconnectTask(
  OpenDDS::DCPS::DummyTcpConnection* connection)
  : connection_ (connection)
{
  DBG_ENTRY_LVL("DummyTcpReconnectTask","DummyTcpReconnectTask",5);
}


OpenDDS::DCPS::DummyTcpReconnectTask::~DummyTcpReconnectTask()
{
  DBG_ENTRY_LVL("DummyTcpReconnectTask","~DummyTcpReconnectTask",5);
}



void OpenDDS::DCPS::DummyTcpReconnectTask::execute (ReconnectOpType& op)
{
  DBG_ENTRY_LVL("DummyTcpReconnectTask","execute",5);

  if (op == DO_RECONNECT)
    {
      if (this->connection_->reconnect () == -1) {
  this->connection_->tear_link ();
      }
    }
  else
    ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: DummyTcpReconnectTask::svc unknown operation %d\n", op));
}
