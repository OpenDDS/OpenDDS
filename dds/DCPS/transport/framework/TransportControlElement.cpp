/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportControlElement.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportControlElement.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportControlElement::TransportControlElement(
  Message_Block_Ptr msg_block
) : TransportQueueElement(1),
    msg_( msg_block.release())
{
  DBG_ENTRY_LVL("TransportControlElement", "TransportControlElement", 6);
}

OpenDDS::DCPS::TransportControlElement::~TransportControlElement()
{
  DBG_ENTRY_LVL("TransportControlElement", "~TransportControlElement", 6);
}

void
OpenDDS::DCPS::TransportControlElement::release_element(
  bool /* dropped_by_transport */
)
{
  delete this;
}
