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
  TransportQueueElement* orig, bool fragment)
  : TransportQueueElement(1),
    orig_(orig),
    publication_id_(orig ? orig->publication_id() : GUID_UNKNOWN),
    fragment_(fragment),
    exclusive_(false)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "TransportCustomizedElement", 6);
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
