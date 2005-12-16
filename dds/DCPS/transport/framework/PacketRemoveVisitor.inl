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


ACE_INLINE
TAO::DCPS::PacketRemoveVisitor::PacketRemoveVisitor
                                    (RepoId              pub_id,
                                     ACE_Message_Block*& unsent_head_block,
                                     ACE_Message_Block*  header_block)
  : sample_(0),
    pub_id_(pub_id),
    head_(unsent_head_block),
    header_block_(header_block),
    status_(0),
    current_block_(0),
    previous_block_(0),
    replaced_element_allocator_(NUM_REPLACED_ELEMENT_CHUNKS)
{
  DBG_ENTRY("PacketRemoveVisitor","PacketRemoveVisitor");
}

