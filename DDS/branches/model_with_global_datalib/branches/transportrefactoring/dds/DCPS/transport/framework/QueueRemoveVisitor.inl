// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::QueueRemoveVisitor::QueueRemoveVisitor
                                       (const ACE_Message_Block* sample)
  : sample_(sample),
    pub_id_(0),
    status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","QueueRemoveVisitor",5);
}

ACE_INLINE
OpenDDS::DCPS::QueueRemoveVisitor::QueueRemoveVisitor(RepoId pub_id)
  : sample_(0),
    pub_id_(pub_id),
    status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","QueueRemoveVisitor",5);
}



ACE_INLINE int
OpenDDS::DCPS::QueueRemoveVisitor::status() const
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","status",5);
  return this->status_;
}


ACE_INLINE int
OpenDDS::DCPS::QueueRemoveVisitor::removed_bytes() const
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","removed_bytes",5);
  return this->removed_bytes_;
}

