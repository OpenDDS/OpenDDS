// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::RemoveAllVisitor::RemoveAllVisitor()
  : status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY_LVL("RemoveAllVisitor","RemoveAllVisitor",5);
}


ACE_INLINE int
OpenDDS::DCPS::RemoveAllVisitor::status() const
{
  DBG_ENTRY_LVL("RemoveAllVisitor","status",5);
  return this->status_;
}


ACE_INLINE int
OpenDDS::DCPS::RemoveAllVisitor::removed_bytes() const
{
  DBG_ENTRY_LVL("RemoveAllVisitor","removed_bytes",5);
  return this->removed_bytes_;
}

