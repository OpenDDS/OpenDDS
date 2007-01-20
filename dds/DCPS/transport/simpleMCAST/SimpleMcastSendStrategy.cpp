// -*- C++ -*-
//
// $Id$
#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "SimpleMcastSendStrategy.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleMcastSendStrategy::~SimpleMcastSendStrategy()
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","~SimpleMcastSendStrategy",5);
}

ssize_t
TAO::DCPS::SimpleMcastSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","send_bytes",5);

  int val = 0;
  ACE_HANDLE handle = this->socket_->get_handle();
  
  ACE::record_and_set_non_blocking_mode(handle, val);
  
  // Set the back-pressure flag to false.
  bp = 0;
  
  // Clear errno
  errno = 0;
  
  ssize_t result = this->socket_->send_bytes(iov, n, this->addr_);
  
  if (result == -1)
    {
      if ((errno == EWOULDBLOCK) || (errno == ENOBUFS))
        {
          VDBG((LM_DEBUG,"DBG:   "
                   "Backpressure encountered.\n"));
          
          // Set the back-pressure flag to true
          bp = 1;
        }
      else
        {
          VDBG_LVL((LM_ERROR, "(%P|%t) SimpleMcastSendStrategy::send_bytes: ERROR: %p iovec count: %d\n",
            "send_bytes", n),1);
          
          // try to get the application to core when "Bad Address" is returned
          // by looking at the iovec
          for (int ii = 0; ii < n; ii++)
            {
              ACE_ERROR((LM_ERROR, "send_bytes: iov[%d].iov_len = %d .iob_base =%X\n",
                ii, iov[ii].iov_len, iov[ii].iov_base ));
            }
        }
    }
    
  VDBG_LVL((LM_DEBUG,"DBG:  The sendv() returned [%d].\n", result), 5);
      
  ACE::restore_non_blocking_mode(handle, val);
  
  return result;
}





