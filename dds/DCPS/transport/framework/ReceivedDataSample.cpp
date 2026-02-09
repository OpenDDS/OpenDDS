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

ReceivedDataSample::ReceivedDataSample()
  : fragment_size_(0)
{
}

ReceivedDataSample::ReceivedDataSample(const ACE_Message_Block& payload)
  : fragment_size_(0)
{
  const ACE_Message_Block* amb = &payload;
  do {
    blocks_.push_back(MessageBlock(*amb));
    amb = amb->cont();
  } while (amb);
}

ReceivedDataSample::ReceivedDataSample(const ReceivedDataSample& other)
  : header_(other.header_)
  , fragment_size_(other.fragment_size_)
  , blocks_(other.blocks_)
{
}

ReceivedDataSample::ReceivedDataSample(ReceivedDataSample&& other)
  : header_(std::move(other.header_))
  , fragment_size_(std::move(other.fragment_size_))
  , blocks_(std::move(other.blocks_))
{
}

ReceivedDataSample& ReceivedDataSample::operator=(const ReceivedDataSample& rhs)
{
  if (this != &rhs) {
    header_ = rhs.header_;
    fragment_size_ = rhs.fragment_size_;
    blocks_ = rhs.blocks_;
  }
  return *this;
}

ReceivedDataSample& ReceivedDataSample::operator=(ReceivedDataSample&& rhs)
{
  if (this != &rhs) {
    header_ = std::move(rhs.header_);
    fragment_size_ = std::move(rhs.fragment_size_);
    blocks_ = std::move(rhs.blocks_);
  }
  return *this;
}

size_t ReceivedDataSample::data_length() const
{
  size_t len = 0;
  for (size_t i = 0; i < blocks_.size(); ++i) {
    len += blocks_[i].len();
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
    mb->data_block(element.duplicate_data());
    mb->rd_ptr(element.rd_ptr());
    mb->wr_ptr(element.wr_ptr());
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
    const unsigned int len = static_cast<unsigned int>(element.len());
    const char* const data = element.rd_ptr();
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
    const unsigned int len = static_cast<unsigned int>(element.len());
    const char* const data = element.rd_ptr();
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
    const size_t len = element.len();
    if (remain < len) {
      return static_cast<unsigned char>(element.rd_ptr()[remain]);
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

void ReceivedDataSample::prepend(ReceivedDataSample&& prefix)
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

void ReceivedDataSample::append(ReceivedDataSample&& suffix)
{
  OPENDDS_MOVE_OR_COPY(suffix.blocks_.begin(), suffix.blocks_.end(),
                       std::back_inserter(blocks_));
  suffix.clear();
}

void ReceivedDataSample::append(const char* data, size_t size)
{
  blocks_.push_back(MessageBlock(size));
  std::memcpy(blocks_.back().base(), data, size);
  blocks_.back().write(size);
}

void ReceivedDataSample::replace(const char* data, size_t size)
{
  clear();
  blocks_.push_back(MessageBlock(data, size));
}

ReceivedDataSample
ReceivedDataSample::get_fragment_range(FragmentNumber start_frag, FragmentNumber end_frag)
{
  ReceivedDataSample result;
  result.header_ = header_;

  const size_t fsize = static_cast<size_t>(fragment_size_);
  const size_t start_offset = static_cast<size_t>(start_frag) * fsize;
  const size_t end_offset = end_frag == INVALID_FRAGMENT ? std::numeric_limits<size_t>::max() : static_cast<size_t>(end_frag + 1) * fsize - 1;

  size_t current_offset = 0;

  bool default_push = false;

  for (OPENDDS_VECTOR(MessageBlock)::iterator it = blocks_.begin(); current_offset < end_offset && it != blocks_.end(); ++it) {
    const size_t len = it->len();
    if (default_push) {
      result.blocks_.push_back(*it);
      if (end_offset < current_offset + len) {
        result.blocks_.back().write(end_offset - current_offset - len); // this should be negative
      }
    } else if (start_offset < current_offset + len) {
      default_push = true;
      result.blocks_.push_back(*it);
      if (current_offset < start_offset) {
        result.blocks_.back().read(start_offset - current_offset);
      }
    }
    current_offset += len;
  }

  return result;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
