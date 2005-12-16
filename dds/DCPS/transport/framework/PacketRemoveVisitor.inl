// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"


ACE_INLINE int
TAO::DCPS::PacketRemoveVisitor::status() const
{
  DBG_ENTRY("PacketRemoveVisitor","status");
  return this->status_;
}

