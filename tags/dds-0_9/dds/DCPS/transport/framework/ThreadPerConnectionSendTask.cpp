// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "ThreadPerConnectionSendTask.h"
#include  "TransportQueueElement.h"
#include  "DataLink.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"



TAO::DCPS::ThreadPerConnectionSendTask::ThreadPerConnectionSendTask(
  TAO::DCPS::DataLink* link)
  : link_ (link)
{
  DBG_ENTRY("ThreadPerConnectionSendTask","ThreadPerConnectionSendTask");
}


TAO::DCPS::ThreadPerConnectionSendTask::~ThreadPerConnectionSendTask()
{
  DBG_ENTRY("ThreadPerConnectionSendTask","~ThreadPerConnectionSendTask");
}



void TAO::DCPS::ThreadPerConnectionSendTask::execute (SendRequest& req)
{
  DBG_ENTRY("ThreadPerConnectionSendTask","execute");

  switch (req.op_)
  {
  case SEND_START:
    this->link_->send_start_i ();
    break;
  case SEND:
    this->link_->send_i (req.element_);
    break;
  case SEND_STOP:
    this->link_->send_stop_i ();
    break;
  default:
    ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: ThreadPerConnectionSendTask::execute unknown command %d\n",
      req.op_));
    break;
  }
}



