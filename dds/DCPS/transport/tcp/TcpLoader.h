/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TCP_LOADER_H
#define TCP_LOADER_H
#include /**/ "ace/pre.h"

#include "Tcp_export.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export TcpLoader : public ACE_Service_Object {
public:

  TcpLoader();

  virtual ~TcpLoader();

  /// Initialize the loader hooks.
  virtual int init(int argc, ACE_TCHAR* argv[]);
};

ACE_STATIC_SVC_DECLARE_EXPORT(OpenDDS_Tcp, TcpLoader)
ACE_FACTORY_DECLARE(OpenDDS_Tcp, TcpLoader)

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"
#endif /* TCP_LOADER_H */
