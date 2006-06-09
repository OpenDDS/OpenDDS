// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::RemoveAllVisitor::RemoveAllVisitor()
  : status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY("RemoveAllVisitor","RemoveAllVisitor");
}


ACE_INLINE int
TAO::DCPS::RemoveAllVisitor::status() const
{
  DBG_ENTRY("RemoveAllVisitor","status");
  return this->status_;
}


ACE_INLINE int
TAO::DCPS::RemoveAllVisitor::removed_bytes() const
{
  DBG_ENTRY("RemoveAllVisitor","removed_bytes");
  return this->removed_bytes_;
}

