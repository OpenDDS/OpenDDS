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
