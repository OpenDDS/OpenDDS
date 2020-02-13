#ifndef MESSAGE_BLOCK_PTR_H_18C6F30C
#define MESSAGE_BLOCK_PTR_H_18C6F30C

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dcps_export.h"
#include "unique_ptr.h"
#include "ace/Message_Block.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct Message_Block_Deleter
{
  void operator()(ACE_Message_Block* ptr) const {
    ACE_Message_Block::release(ptr);
  }
};

typedef unique_ptr<ACE_Message_Block, Message_Block_Deleter> Message_Block_Ptr;

class OpenDDS_Dcps_Export Message_Block_Shared_Ptr {
public:
  Message_Block_Shared_Ptr() {}

  explicit Message_Block_Shared_Ptr(ACE_Message_Block* payload)
    : msg_(payload)
  {}

  Message_Block_Shared_Ptr(const Message_Block_Shared_Ptr& other)
    : msg_(ACE_Message_Block::duplicate(other.msg_.get()))
  {}

  Message_Block_Shared_Ptr& operator=(const Message_Block_Shared_Ptr& other)
  {
    Message_Block_Shared_Ptr cpy(other);
    swap(*this, cpy);
    return *this;
  }

  operator bool() const { return msg_.get(); }

  ACE_Message_Block& operator*() const { return *msg_; }

  ACE_Message_Block* operator->() const { return msg_.get(); }

  ACE_Message_Block* get() const { return msg_.get(); }

  static void swap(Message_Block_Shared_Ptr& a, Message_Block_Shared_Ptr& b)
  {
    using std::swap;
    swap(a.msg_, b.msg_);
  }

 private:
  Message_Block_Ptr msg_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // MESSAGE_BLOCK_PTR_H_18C6F30C
