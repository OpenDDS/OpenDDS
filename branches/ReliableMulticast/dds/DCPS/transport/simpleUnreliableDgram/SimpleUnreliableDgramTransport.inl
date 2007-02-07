// -*- C++ -*-
//
// $Id$

#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUnreliableDgramTransport::SimpleUnreliableDgramTransport()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","SimpleUnreliableDgramTransport",5);
}


ACE_INLINE void
TAO::DCPS::SimpleUnreliableDgramTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","deliver_sample",5);

  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(remote_address);

  // The subclass should always override this function so 
  // this function should not be called.
  ACE_ERROR ((LM_ERROR, "(%P|%t)SimpleUnreliableDgramTransport::deliver_sample "
    "should not be called. \n"));
}


ACE_INLINE bool
TAO::DCPS::SimpleUnreliableDgramTransport::acked (RepoId)
{
  return true;
}


