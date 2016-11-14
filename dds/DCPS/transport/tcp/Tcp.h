/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCP_H
#define OPENDDS_TCP_H

#include /**/ "ace/pre.h"

#include "Tcp_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"
#include "dds/Versioned_Namespace.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export Tcp_Initializer {
public:
  // Constructor.
  Tcp_Initializer();
};

static Tcp_Initializer tcp_initializer;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_TCP_H */
