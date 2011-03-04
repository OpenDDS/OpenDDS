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
  TransportSendElement* tse, ACE_Allocator* allocator)
  : TransportQueueElement(1),
    tse_(tse),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "TransportCustomizedElement", 6);
}

ACE_INLINE /*static*/
TransportCustomizedElement*
TransportCustomizedElement::alloc(TransportSendElement* tse)
{
  TransportCustomizedElement* ret;
  TransportCustomizedElementAllocator* al =
    tse->sample()->transport_customized_element_allocator_;
  ACE_NEW_MALLOC_RETURN(ret,
    static_cast<TransportCustomizedElement*>(al->malloc()),
    TransportCustomizedElement(tse, al),
    0);
  return ret;
}

ACE_INLINE
bool
TransportCustomizedElement::owned_by_transport ()
{
  return false;
}

} // namespace DCPS
} // namespace OpenDDS
