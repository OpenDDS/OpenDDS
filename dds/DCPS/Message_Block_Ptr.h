#ifndef MESSAGE_BLOCK_PTR_H_18C6F30C
#define MESSAGE_BLOCK_PTR_H_18C6F30C

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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

typedef unique_ptr<ACE_Message_Block,Message_Block_Deleter> Message_Block_Ptr;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // MESSAGE_BLOCK_PTR_H_18C6F30C
