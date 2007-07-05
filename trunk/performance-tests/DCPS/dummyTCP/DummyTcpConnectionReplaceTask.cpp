// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpConnectionReplaceTask.h"
#include "DummyTcpTransport.h"
#include "DummyTcpConnection.h"
#include "DummyTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"



OpenDDS::DCPS::DummyTcpConnectionReplaceTask::DummyTcpConnectionReplaceTask(
  DummyTcpTransport* trans)
  : trans_ (trans)
{
  DBG_ENTRY_LVL("DummyTcpConnectionReplaceTask","DummyTcpConnectionReplaceTask",5);
}


OpenDDS::DCPS::DummyTcpConnectionReplaceTask::~DummyTcpConnectionReplaceTask()
{
  DBG_ENTRY_LVL("DummyTcpConnectionReplaceTask","~DummyTcpConnectionReplaceTask",5);
}



void OpenDDS::DCPS::DummyTcpConnectionReplaceTask::execute (DummyTcpConnection_rch& con)
{
  DBG_ENTRY_LVL("DummyTcpConnectionReplaceTask","execute",5);

  this->trans_->fresh_link (con->get_remote_address(), con);
}




