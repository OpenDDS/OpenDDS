/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TransportCustomizedElement::TransportCustomizedElement(
  TransportQueueElement* orig, bool fragment, ACE_Allocator* allocator)
  : TransportQueueElement(1),
    orig_(orig),
    msg_(0),
    allocator_(allocator),
    publication_id_(orig ? orig->publication_id() : GUID_UNKNOWN),
    fragment_(fragment),
    exclusive_(false)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "TransportCustomizedElement", 6);
}

ACE_INLINE /*static*/
TransportCustomizedElement*
TransportCustomizedElement::alloc(TransportQueueElement* orig,
                                  bool fragment /* = false */,
                                  ACE_Allocator* allocator /* = 0 */)
{
  if (allocator) {
    TransportCustomizedElement* ret;
    ACE_NEW_MALLOC_RETURN(ret,
      static_cast<TransportCustomizedElement*>(allocator->malloc(0)),
      TransportCustomizedElement(orig, fragment, allocator),
      0);
    return ret;
  } else {
    return new TransportCustomizedElement(orig, fragment, 0);
  }
}

ACE_INLINE
SequenceNumber
TransportCustomizedElement::sequence() const
{
  return this->orig_ ? this->orig_->sequence()
    : SequenceNumber::SEQUENCENUMBER_UNKNOWN();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
