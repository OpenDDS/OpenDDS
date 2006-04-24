// -*- C++ -*-
//
// $Id$
#include  "SimpleUdpConfiguration.h"
#include  "SimpleUdpSocket.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpSendStrategy::SimpleUdpSendStrategy
                                     (SimpleUdpConfiguration* config,
                                      const ACE_INET_Addr&    remote_address,
                                      SimpleUdpSocket*        socket)
  : TransportSendStrategy(config,0),
    addr_(remote_address)
{
  DBG_ENTRY("SimpleUdpSendStrategy","SimpleUdpSendStrategy");

  // Keep a "copy" of the reference to the SimpleUdpSocket object
  // for ourselves.
  socket->_add_ref();
  this->socket_ = socket;
}


ACE_INLINE
void
TAO::DCPS::SimpleUdpSendStrategy::stop_i()
{
  DBG_ENTRY("SimpleUdpSendStrategy","stop_i");
  //TODO: noop
}

