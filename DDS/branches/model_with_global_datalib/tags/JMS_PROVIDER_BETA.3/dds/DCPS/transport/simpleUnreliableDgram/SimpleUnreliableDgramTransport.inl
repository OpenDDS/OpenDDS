// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::SimpleUnreliableDgramTransport::SimpleUnreliableDgramTransport()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramTransport","SimpleUnreliableDgramTransport",6);
}


ACE_INLINE void
OpenDDS::DCPS::SimpleUnreliableDgramTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address,
                                      CORBA::Long          /* priority */)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","deliver_sample",6);

  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(remote_address);

  // The subclass should always override this function so 
  // this function should not be called.
  ACE_ERROR ((LM_ERROR, "(%P|%t)SimpleUnreliableDgramTransport::deliver_sample "
    "should not be called. \n"));
}


ACE_INLINE bool
OpenDDS::DCPS::SimpleUnreliableDgramTransport::acked (RepoId, RepoId)
{
  return true;
}


ACE_INLINE void 
OpenDDS::DCPS::SimpleUnreliableDgramTransport::remove_ack (RepoId /*pub_id*/, RepoId /*sub_id*/)
{
  //noop
}



