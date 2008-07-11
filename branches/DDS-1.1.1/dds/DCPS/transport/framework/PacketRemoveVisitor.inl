// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"


ACE_INLINE int
OpenDDS::DCPS::PacketRemoveVisitor::status() const
{
  DBG_ENTRY_LVL("PacketRemoveVisitor","status",6);
  return this->status_;
}

