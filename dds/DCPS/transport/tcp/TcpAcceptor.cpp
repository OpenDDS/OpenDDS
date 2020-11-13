/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpAcceptor.h"
#include "TcpTransport.h"
#include "TcpSendStrategy.h"
#include "TcpInst.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TcpAcceptor::TcpAcceptor(RcHandle<TcpTransport> transport)
  : transport_(transport)
{
  DBG_ENTRY_LVL("TcpAcceptor","TcpAcceptor",6);
}

TcpAcceptor::~TcpAcceptor()
{
  DBG_ENTRY_LVL("TcpAcceptor","~TcpAcceptor",6);
  transport_.reset();
}

RcHandle<TcpTransport>
TcpAcceptor::transport()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport",6);
  return transport_.lock();
}

void
TcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport_shutdown",6);
  transport_.reset();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
