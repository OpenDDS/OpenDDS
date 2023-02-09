/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ReceivedDataSample.h"

#include <algorithm>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

ReceivedDataSample::ReceivedDataSample(const ACE_Message_Block& payload)
{
  const ACE_Message_Block* amb = &payload;
  do {
    blocks_.push_back(MessageBlock(*amb));
    amb = amb->cont();
  } while (amb);
}

size_t ReceivedDataSample::data_length() const
{
  size_t len = 0;
  for (size_t i = 0; i < blocks_.size(); ++i) {
    len += blocks_[i].wr_ptr_ - blocks_[i].rd_ptr_;
  }
  return len;
}

namespace {
  ACE_Message_Block* make_mb(ACE_Allocator* mb_alloc)
  {
    if (!mb_alloc) {
      mb_alloc = ACE_Allocator::instance();
    }
    ACE_Message_Block* mb = 0;
    ACE_NEW_MALLOC_RETURN(mb,
      (ACE_Message_Block*) mb_alloc->malloc(sizeof(ACE_Message_Block)),
      ACE_Message_Block(mb_alloc),
      0);
    return mb;
  }
}

ACE_Message_Block* ReceivedDataSample::data(ACE_Allocator* mb_alloc) const
{
  ACE_Message_Block* first = 0;
  ACE_Message_Block* last = 0;
  for (size_t i = 0; i < blocks_.size(); ++i) {
    const MessageBlock& element = blocks_[i];
    ACE_Message_Block* const mb = make_mb(mb_alloc);
    mb->data_block(element.data_->duplicate());
    mb->rd_ptr(element.rd_ptr_);
    mb->wr_ptr(element.wr_ptr_);
    if (first) {
      last->cont(mb);
    } else {
      first = mb;
    }
    last = mb;
  }
  return first;
}

bool ReceivedDataSample::write_data(Serializer& ser) const
{
  for (size_t i = 0; i < blocks_.size(); ++i) {
    const MessageBlock& element = blocks_[i];
    const unsigned int len = static_cast<unsigned int>(element.wr_ptr_ - element.rd_ptr_);
    const char* const data = element.data_->base() + element.rd_ptr_;
    if (!ser.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(data), len)) {
      return false;
    }
  }
  return true;
}

DDS::OctetSeq ReceivedDataSample::copy_data() const
{
  DDS::OctetSeq dst;
  dst.length(static_cast<unsigned int>(data_length()));
  unsigned char* out_iter = dst.get_buffer();
  for (size_t i = 0; i < blocks_.size(); ++i) {
    const MessageBlock& element = blocks_[i];
    const unsigned int len = static_cast<unsigned int>(element.wr_ptr_ - element.rd_ptr_);
    const char* const data = element.data_->base() + element.rd_ptr_;
    std::memcpy(out_iter, data, len);
    out_iter += len;
  }
  return dst;
}

unsigned char ReceivedDataSample::peek(size_t offset) const
{
  size_t remain = offset;
  for (size_t i = 0; i < blocks_.size(); ++i) {
    const MessageBlock& element = blocks_[i];
    const size_t len = element.wr_ptr_ - element.rd_ptr_;
    if (remain < len) {
      return element.data_->base()[element.rd_ptr_ + remain];
    }
    remain -= len;
  }
  return 0;
}

#ifdef ACE_HAS_CPP11
#define OPENDDS_MOVE_OR_COPY std::move
#else
#define OPENDDS_MOVE_OR_COPY std::copy
#endif

void ReceivedDataSample::prepend(ReceivedDataSample& prefix)
{
  OPENDDS_MOVE_OR_COPY(prefix.blocks_.begin(), prefix.blocks_.end(),
                       std::inserter(blocks_, blocks_.begin()));
  prefix.clear();
}

void ReceivedDataSample::append(ReceivedDataSample& suffix)
{
  OPENDDS_MOVE_OR_COPY(suffix.blocks_.begin(), suffix.blocks_.end(),
                       std::back_inserter(blocks_));
  suffix.clear();
}

void ReceivedDataSample::append(const char* data, size_t size)
{
  blocks_.push_back(MessageBlock(size));
  std::memcpy(blocks_.back().data_->base(), data, size);
  blocks_.back().wr_ptr_ = size;
}

void ReceivedDataSample::replace(const char* data, size_t size)
{
  clear();
  blocks_.push_back(MessageBlock(data, size));
}

ReceivedDataSample
ReceivedDataSample::get_fragment_range(ReceivedDataSample::FragmentNumber start_frag, ReceivedDataSample::FragmentNumber end_frag)
{
  ReceivedDataSample result;
  result.header_ = header_;

  const ACE_UINT16 fsize = header_.fragment_size_;
  const size_t start_offset = start_frag * fsize;
  const size_t end_offset = end_frag == INVALID_FRAGMENT ? std::numeric_limits<size_t>::max() : (end_frag + 1) * fsize - 1;

  size_t current_offset = 0;

  bool default_push = false;

  for (OPENDDS_VECTOR(MessageBlock)::iterator it = blocks_.begin(); current_offset < end_offset && it != blocks_.end(); ++it) {
    const size_t len = it->wr_ptr_ - it->rd_ptr_;
    if (default_push) {
      result.blocks_.push_back(*it);
      if (end_offset < current_offset + len) {
        result.blocks_.back().wr_ptr_ -= (current_offset + len - end_offset);
      }
    } else if (start_offset < current_offset + len) {
      default_push = true;
      result.blocks_.push_back(*it);
      if (current_offset < start_offset) {
        result.blocks_.back().rd_ptr_ += start_offset - current_offset;
      }
    }
    current_offset += len;
  }

  return result;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
