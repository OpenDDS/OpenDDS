/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::TransportSendElement::TransportSendElement(int initial_count,
                                                          const DataSampleElement* sample)
  : TransportQueueElement(initial_count),
    element_(sample)
{
  DBG_ENTRY_LVL("TransportSendElement", "TransportSendElement", 6);
}


ACE_INLINE
bool
OpenDDS::DCPS::TransportSendElement::owned_by_transport()
{
  return false;
}

ACE_INLINE
OpenDDS::DCPS::SequenceNumber
OpenDDS::DCPS::TransportSendElement::sequence() const
{
  return element_->get_header().sequence_;
}

ACE_INLINE
const OpenDDS::DCPS::DataSampleElement*
OpenDDS::DCPS::TransportSendElement::sample() const
{
  return element_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
