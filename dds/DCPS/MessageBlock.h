#ifndef OPENDDS_DCPS_MESSAGEBLOCK_H
#define OPENDDS_DCPS_MESSAGEBLOCK_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dcps_export.h"

#include <dds/Versioned_Namespace.h>

#include <ace/Message_Block.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MessageBlock {
public:
  /// construct a MessageBlock that references the existing amb's ACE_Data_Block
  explicit MessageBlock(const ACE_Message_Block& amb);

  /// construct a MessageBlock with 'size' bytes allocated but not used (wr_ptr_ is 0)
  explicit MessageBlock(size_t size);

  /// construct a MessageBlock that points to external data, doesn't allocate or copy
  MessageBlock(const char* data, size_t size);

  ~MessageBlock();

  MessageBlock(const MessageBlock& rhs);
  MessageBlock& operator=(const MessageBlock& rhs);
#ifdef ACE_HAS_CPP11
  MessageBlock(MessageBlock&& rhs);
  MessageBlock& operator=(MessageBlock&& rhs);
#endif

  void swap(MessageBlock& rhs);

  ACE_Data_Block* duplicate_data() const { return data_->duplicate(); }

  char* base() const { return data_->base(); }

  char* rd_ptr() const { return base() + rd_ptr_; }
  void read(size_t diff) { rd_ptr_ += diff; }
  void unread(size_t diff) { rd_ptr_ -= diff; }

  char* wr_ptr() const { return base() + wr_ptr_; }
  void write(size_t diff) { wr_ptr_ += diff; }
  void unwrite(size_t diff) { wr_ptr_ -= diff; }

  size_t len() const { return wr_ptr_ - rd_ptr_; }

private:
  ACE_Data_Block* data_;
  size_t rd_ptr_;
  size_t wr_ptr_;
};

void swap(MessageBlock& lhs, MessageBlock& rhs);

} // DCPS
} // OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_MESSAGEBLOCK_H */
