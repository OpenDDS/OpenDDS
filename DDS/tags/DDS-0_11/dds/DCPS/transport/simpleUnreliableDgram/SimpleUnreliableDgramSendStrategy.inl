// -*- C++ -*-
//
// $Id$
#include  "SimpleUnreliableDgramSocket.h"
#include  "SimpleUnreliableDgramSynchResource.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"


ACE_INLINE
TAO::DCPS::SimpleUnreliableDgramSendStrategy::SimpleUnreliableDgramSendStrategy
                                     (TransportConfiguration* config,
                                      const ACE_INET_Addr&    remote_address,
                                      SimpleUnreliableDgramSocket*        socket,
                                      SimpleUnreliableDgramSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource),
    remote_address_(remote_address)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSendStrategy","SimpleUnreliableDgramSendStrategy",5);

  // Keep a "copy" of the reference to the SimpleUdpSocket object
  // for ourselves.
  socket->_add_ref();
  this->socket_ = socket;
}


ACE_INLINE
void
TAO::DCPS::SimpleUnreliableDgramSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSendStrategy","stop_i",5);
  //TODO: noop
}

