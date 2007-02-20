// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::EventHandler::~EventHandler()
{
  close();
}
