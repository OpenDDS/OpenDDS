/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "Tcp.h"
#include "TcpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Tcp_Initializer::Tcp_Initializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_TcpLoader);

#if OPENDDS_TCP_HAS_DLL == 0
  ACE_Service_Config::process_directive(ACE_TEXT("static OpenDDS_Tcp"));
#endif
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
