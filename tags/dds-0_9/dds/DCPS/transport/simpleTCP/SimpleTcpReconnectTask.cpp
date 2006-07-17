// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpReconnectTask.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"



TAO::DCPS::SimpleTcpReconnectTask::SimpleTcpReconnectTask(
  TAO::DCPS::SimpleTcpConnection* connection)
  : connection_ (connection)
{
  DBG_ENTRY("SimpleTcpReconnectTask","SimpleTcpReconnectTask");
}


TAO::DCPS::SimpleTcpReconnectTask::~SimpleTcpReconnectTask()
{
  DBG_ENTRY("SimpleTcpReconnectTask","~SimpleTcpReconnectTask");
}



void TAO::DCPS::SimpleTcpReconnectTask::execute (ReconnectOpType& op)
{
  DBG_ENTRY("SimpleTcpReconnectTask","execute");

  if (op == DO_RECONNECT)
    this->connection_->reconnect ();
  else
    ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: SimpleTcpReconnectTask::svc unknown operation %d\n", op));
}




