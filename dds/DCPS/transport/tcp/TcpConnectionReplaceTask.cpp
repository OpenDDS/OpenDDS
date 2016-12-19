/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpConnectionReplaceTask.h"
#include "TcpTransport.h"
#include "TcpConnection.h"
#include "TcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpConnectionReplaceTask::TcpConnectionReplaceTask(
  TcpTransport* trans)
  : trans_(trans)
{
  DBG_ENTRY_LVL("TcpConnectionReplaceTask","TcpConnectionReplaceTask",6);
}

OpenDDS::DCPS::TcpConnectionReplaceTask::~TcpConnectionReplaceTask()
{
  DBG_ENTRY_LVL("TcpConnectionReplaceTask","~TcpConnectionReplaceTask",6);
}

void OpenDDS::DCPS::TcpConnectionReplaceTask::execute(TcpConnection_rch& con)
{
  DBG_ENTRY_LVL("TcpConnectionReplaceTask","execute",6);

  this->trans_->fresh_link(con);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
