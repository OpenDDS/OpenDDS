// -*- C++ -*-
//
// $Id$
#include  "SimpleUdpConfiguration.h"
#include  "SimpleUdpSocket.h"
#include  "SimpleUdpSynchResource.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpSendStrategy::SimpleUdpSendStrategy
                                     (SimpleUdpConfiguration* config,
                                      const ACE_INET_Addr&    remote_address,
                                      SimpleUdpSocket*        socket,
                                      SimpleUdpSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource),
    addr_(remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpSendStrategy","SimpleUdpSendStrategy",5);

  // Keep a "copy" of the reference to the SimpleUdpSocket object
  // for ourselves.
  socket->_add_ref();
  this->socket_ = socket;
}


ACE_INLINE
void
TAO::DCPS::SimpleUdpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleUdpSendStrategy","stop_i",5);
  //TODO: noop
}

