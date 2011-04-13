/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TransportCustomizedElement::TransportCustomizedElement(
  TransportQueueElement* orig, ACE_Allocator* allocator)
  : TransportQueueElement(1),
    orig_(orig),
    allocator_(allocator),
    publication_id_(orig ? orig->publication_id() : GUID_UNKNOWN)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "TransportCustomizedElement", 6);
}

ACE_INLINE /*static*/
TransportCustomizedElement*
TransportCustomizedElement::alloc(TransportQueueElement* orig,
                                  ACE_Allocator* allocator /* = 0*/)
{
  if (allocator) {
    TransportCustomizedElement* ret;
    ACE_NEW_MALLOC_RETURN(ret,
      static_cast<TransportCustomizedElement*>(allocator->malloc(0)),
      TransportCustomizedElement(orig, allocator),
      0);
    return ret;
  } else {
    return new TransportCustomizedElement(orig, 0);
  }
}

ACE_INLINE
bool
TransportCustomizedElement::owned_by_transport()
{
  return false;
}

} // namespace DCPS
} // namespace OpenDDS
