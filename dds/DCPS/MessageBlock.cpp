/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "MessageBlock.h"

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MessageBlock::MessageBlock(size_t size)
  : data_()
  , rd_ptr_()
  , wr_ptr_()
{
  ACE_Allocator* const alloc = ACE_Allocator::instance();
  ACE_NEW_MALLOC(data_,
    (ACE_Data_Block*) alloc->malloc(sizeof(ACE_Data_Block)),
    ACE_Data_Block(size, ACE_Message_Block::MB_DATA, 0, alloc, 0,
                   0, alloc));
}

MessageBlock::MessageBlock(const char* data, size_t size)
  : data_()
  , rd_ptr_()
  , wr_ptr_(size)
{
  ACE_Allocator* const alloc = ACE_Allocator::instance();
  ACE_NEW_MALLOC(data_,
    (ACE_Data_Block*) alloc->malloc(sizeof(ACE_Data_Block)),
    ACE_Data_Block(size, ACE_Message_Block::MB_DATA, data, alloc, 0,
                   ACE_Message_Block::DONT_DELETE, alloc));
}

MessageBlock::MessageBlock(const ACE_Message_Block& amb)
  : data_(amb.data_block()->duplicate())
  , rd_ptr_(amb.rd_ptr() - amb.base())
  , wr_ptr_(amb.wr_ptr() - amb.base())
{}

MessageBlock::MessageBlock(const MessageBlock& rhs)
  : data_(rhs.data_->duplicate())
  , rd_ptr_(rhs.rd_ptr_)
  , wr_ptr_(rhs.wr_ptr_)
{}

MessageBlock::~MessageBlock()
{
  if (data_) {
    data_->release();
  }
}

void
MessageBlock::swap(MessageBlock& rhs)
{
  std::swap(data_, rhs.data_);
  std::swap(rd_ptr_, rhs.rd_ptr_);
  std::swap(wr_ptr_, rhs.wr_ptr_);
}

MessageBlock&
MessageBlock::operator=(const MessageBlock& rhs)
{
  if (&rhs != this) {
    MessageBlock copy(rhs);
    swap(copy);
  }
  return *this;
}

#ifdef ACE_HAS_CPP11
MessageBlock::MessageBlock(MessageBlock&& rhs)
  : data_(rhs.data_)
  , rd_ptr_(rhs.rd_ptr_)
  , wr_ptr_(rhs.wr_ptr_)
{
  rhs.data_ = 0;
}

MessageBlock&
MessageBlock::operator=(MessageBlock&& rhs)
{
  swap(rhs);
  return *this;
}
#endif

void swap(MessageBlock& lhs, MessageBlock& rhs)
{
  lhs.swap(rhs);
}

} // DCPS
} // OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
