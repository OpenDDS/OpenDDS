// -*- C++ -*-
//
// $Id$
#include  "SimpleMcastConfiguration.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastSynchResource.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleMcastSendStrategy::SimpleMcastSendStrategy
                                     (SimpleMcastConfiguration* config,
                                      const ACE_INET_Addr&    remote_address,
                                      SimpleMcastSocket*        socket,
                                      SimpleMcastSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource),
    addr_(remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","SimpleMcastSendStrategy",5);

  // Keep a "copy" of the reference to the SimpleMcastSocket object
  // for ourselves.
  socket->_add_ref();
  this->socket_ = socket;
}


ACE_INLINE
void
TAO::DCPS::SimpleMcastSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","stop_i",5);
  //TODO: noop
}

