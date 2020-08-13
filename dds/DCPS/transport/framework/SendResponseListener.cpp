/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "SendResponseListener.h"

#include "ace/Message_Block.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

SendResponseListener::SendResponseListener(const OPENDDS_STRING& msg_src)
: tracker_(msg_src)
{
}

SendResponseListener::~SendResponseListener()
{
  tracker_.wait_messages_pending("SendResponseListener::~SendResponseListener");
}

void
SendResponseListener::data_delivered(const DataSampleElement* /* sample */)
{
  tracker_.message_delivered();
}

void
SendResponseListener::data_dropped(
  const DataSampleElement* /* sample */,
  bool /* dropped_by_transport */)
{
  tracker_.message_dropped();
}

void
SendResponseListener::control_delivered(const Message_Block_Ptr& /* sample */)
{
  tracker_.message_delivered();
}

void
SendResponseListener::control_dropped(
  const Message_Block_Ptr& /* sample */,
  bool /* dropped_by_transport */)
{
  tracker_.message_dropped();
}

void
SendResponseListener::track_message()
{
  tracker_.message_sent();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
