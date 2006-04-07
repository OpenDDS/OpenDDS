// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpSendStrategy.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpSendStrategy.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleTcpSendStrategy::~SimpleTcpSendStrategy()
{
  DBG_ENTRY("SimpleTcpSendStrategy","~SimpleTcpSendStrategy");
}


ssize_t
TAO::DCPS::SimpleTcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY("SimpleTcpSendStrategy","send_bytes");

  int val = 0;
  ACE_HANDLE handle = this->connection_->peer().get_handle();

  ACE::record_and_set_non_blocking_mode(handle, val);

  // Set the back-pressure flag to false.
  bp = 0;

  // Clear errno
  errno = 0;

  ssize_t result = this->connection_->peer().sendv(iov, n);

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
          ACE_ERROR((LM_ERROR, "SimpleTcpSendStrategy::send_bytes: ERROR: %p iovec count: %d\n",
            "sendv", n));

          // try to get the application to core when "Bad Address" is returned
          // by looking at the iovec
          for (int ii = 0; ii < n; ii++) 
            {
              ACE_ERROR((LM_ERROR, "send_bytes: iov[%d].iov_len = %d .iob_base =%X\n",
                ii, iov[ii].iov_len, iov[ii].iov_base ));
            }
        }
    }

  VDBG((LM_DEBUG,"DBG:   "
             "The sendv() returned [%d].\n", result));

  ACE::restore_non_blocking_mode(handle, val);

  return result;
}


int 
TAO::DCPS::SimpleTcpSendStrategy::relink ()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","relink");
  return this->connection_->reconnect ();
}


