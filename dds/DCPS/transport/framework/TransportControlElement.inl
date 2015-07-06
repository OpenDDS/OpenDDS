/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
bool
OpenDDS::DCPS::TransportControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportControlElement","requires_exclusive_packet",6);
  return true;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportControlElement::data_delivered()
{
  DBG_ENTRY_LVL("TransportControlElement","data_delivered",6);
}

ACE_INLINE
OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportControlElement::publication_id() const
{
  return GUID_UNKNOWN;
}

ACE_INLINE
const ACE_Message_Block*
OpenDDS::DCPS::TransportControlElement::msg() const
{
  return this->msg_;
}

ACE_INLINE
const ACE_Message_Block*
OpenDDS::DCPS::TransportControlElement::msg_payload() const
{
  return this->msg_->cont();
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportControlElement::owned_by_transport()
{
  return true;
}

