// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"


ACE_INLINE int
TAO::DCPS::PacketRemoveVisitor::status() const
{
  DBG_ENTRY_LVL("PacketRemoveVisitor","status",5);
  return this->status_;
}

