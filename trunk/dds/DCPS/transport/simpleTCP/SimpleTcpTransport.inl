// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpSendStrategy.h"
#include  "SimpleTcpReceiveStrategy.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpSynchResource.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpReconnectTask.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpTransport::SimpleTcpTransport()
  : acceptor_(this),
    connections_updated_(this->connections_lock_),
    reconnect_task_ (new SimpleTcpReconnectTask (this))
{
  DBG_ENTRY("SimpleTcpTransport","SimpleTcpTransport");
}


ACE_INLINE       
TAO::DCPS::SimpleTcpReconnectTask_rch 
TAO::DCPS::SimpleTcpTransport::get_reconnect_task ()
{
  return this->reconnect_task_;
}


