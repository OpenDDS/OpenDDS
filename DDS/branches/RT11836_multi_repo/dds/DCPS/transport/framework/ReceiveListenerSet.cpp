// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReceiveListenerSet.h"


#if !defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::ReceiveListenerSet::~ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet","~ReceiveListenerSet",5);
}


bool 
OpenDDS::DCPS::ReceiveListenerSet::exist (const RepoId& local_id, 
                                      bool& last)
{
  GuardType guard(this->lock_);

  last = true;

  TransportReceiveListener* listener = 0;
  if (find(map_, local_id, listener) == -1)
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

  last = map_.size() == 1;
  return true;
}


void 
OpenDDS::DCPS::ReceiveListenerSet::get_keys (ReaderIdSeq & ids)
{
  GuardType guard(this->lock_);
  for (MapType::iterator iter = map_.begin();
    iter != map_.end(); ++ iter)
  {
    CORBA::ULong sz = ids.length ();
    ids.length (sz + 1);
    ids[sz] = iter->first;
  }
}

bool 
OpenDDS::DCPS::ReceiveListenerSet::exist (const RepoId& local_id)
{
  GuardType guard(this->lock_);

  TransportReceiveListener* listener = 0;
  return (find(map_, local_id, listener) == -1 ? false : true);
}


