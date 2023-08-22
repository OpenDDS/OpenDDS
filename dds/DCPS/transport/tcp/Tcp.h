/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TCP_TCP_H
#define OPENDDS_DCPS_TRANSPORT_TCP_TCP_H

#include "Tcp_export.h"

#include <dds/Versioned_Namespace.h>

#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

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

#endif /* OPENDDS_TCP_H */
