// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReceiveListenerSet.h"


#if !defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::ReceiveListenerSet::~ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet","~ReceiveListenerSet",5);
}


bool 
TAO::DCPS::ReceiveListenerSet::exist (const RepoId& local_id, 
                                      bool& last)
{
  last = true;

  TransportReceiveListener* listener;
  if (this->map_.find(local_id, listener) == -1)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t)ReceiveListenerSet::exist could not find local %d \n",
      local_id));

    return false;
  }

  if (listener == 0)
  {
     ACE_ERROR ((LM_ERROR, "(%P|%t)ReceiveListenerSet::exist listener for local %d is nil\n",
       local_id));

     return false;
  }

  last = this->map_.current_size() == 1;
  return true;
}



