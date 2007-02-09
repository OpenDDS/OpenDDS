// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::EventHandler::~EventHandler()
{
  handle_close(ACE_INVALID_HANDLE, 0);
}
