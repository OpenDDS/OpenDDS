// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::QueueRemoveVisitor::QueueRemoveVisitor
                                       (const ACE_Message_Block* sample)
  : sample_(sample),
    pub_id_(0),
    status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY("QueueRemoveVisitor","QueueRemoveVisitor");
}

ACE_INLINE
TAO::DCPS::QueueRemoveVisitor::QueueRemoveVisitor(RepoId pub_id)
  : sample_(0),
    pub_id_(pub_id),
    status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY("QueueRemoveVisitor","QueueRemoveVisitor");
}



ACE_INLINE int
TAO::DCPS::QueueRemoveVisitor::status() const
{
  DBG_ENTRY("QueueRemoveVisitor","status");
  return this->status_;
}


ACE_INLINE int
TAO::DCPS::QueueRemoveVisitor::removed_bytes() const
{
  DBG_ENTRY("QueueRemoveVisitor","removed_bytes");
  return this->removed_bytes_;
}

