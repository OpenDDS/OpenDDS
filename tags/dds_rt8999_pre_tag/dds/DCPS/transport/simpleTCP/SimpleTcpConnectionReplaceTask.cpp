// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpConnectionReplaceTask.h"
#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"



TAO::DCPS::SimpleTcpConnectionReplaceTask::SimpleTcpConnectionReplaceTask(
  SimpleTcpTransport* trans)
  : trans_ (trans)
{
  DBG_ENTRY("SimpleTcpConnectionReplaceTask","SimpleTcpConnectionReplaceTask");
}


TAO::DCPS::SimpleTcpConnectionReplaceTask::~SimpleTcpConnectionReplaceTask()
{
  DBG_ENTRY("SimpleTcpConnectionReplaceTask","~SimpleTcpConnectionReplaceTask");
}



void TAO::DCPS::SimpleTcpConnectionReplaceTask::execute (SimpleTcpConnection_rch& con)
{
  DBG_ENTRY("SimpleTcpConnectionReplaceTask","execute");

  this->trans_->fresh_link (con->get_remote_address(), con);
}




