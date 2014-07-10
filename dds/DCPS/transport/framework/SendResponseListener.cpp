/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "SendResponseListener.h"

#include "ace/Message_Block.h"

namespace OpenDDS {
namespace DCPS {

SendResponseListener::SendResponseListener(const std::string& msg_src)
: tracker_(msg_src)
{
}

SendResponseListener::~SendResponseListener()
{
  tracker_.wait_messages_pending();
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
SendResponseListener::control_delivered(ACE_Message_Block* sample)
{
  if (sample != 0) sample->release();
  tracker_.message_delivered();
}

void
SendResponseListener::control_dropped(
  ACE_Message_Block* sample,
  bool /* dropped_by_transport */)
{
  if (sample != 0) sample->release();
  tracker_.message_dropped();
}

void
SendResponseListener::track_message()
{
  tracker_.message_sent();
}

} // namespace DCPS
} // namespace OpenDDS
