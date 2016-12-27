/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "dds/DCPS/DataSampleElement.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE /*static*/
TransportSendControlElement*
TransportSendControlElement::alloc(int initial_count,
                                   const RepoId& publisher_id,
                                   TransportSendListener* listener,
                                   const DataSampleHeader& header,
                                   ACE_Message_Block* message,
                                   TransportSendControlElementAllocator* allocator)
{
  TransportSendControlElement* ret;
  ACE_NEW_MALLOC_RETURN(ret,
    static_cast<TransportSendControlElement*>(allocator->malloc()),
    TransportSendControlElement(initial_count, publisher_id, listener, header,
                                message, allocator),
    0);
  return ret;
}

ACE_INLINE /*static*/
TransportSendControlElement*
TransportSendControlElement::alloc(int initial_count,
                                   const DataSampleElement* dcps_elem,
                                   TransportSendControlElementAllocator* allocator)
{
  TransportSendControlElement* ret;
  ACE_NEW_MALLOC_RETURN(ret,
    static_cast<TransportSendControlElement*>(allocator->malloc()),
    TransportSendControlElement(initial_count, dcps_elem, allocator),
    0);
  return ret;
}

ACE_INLINE
bool
TransportSendControlElement::owned_by_transport()
{
  return false;
}

ACE_INLINE
SequenceNumber
TransportSendControlElement::sequence() const
{
  return this->header_.sequence_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
