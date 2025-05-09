/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_UTILITYIMPL_H
#define OPENDDS_DCPS_SECURITY_UTILITYIMPL_H

#include "OpenDDS_Security_Export.h"
#include "framework/Utility.h"

#include <dds/Versioned_Namespace.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Security_Export UtilityImpl
  : public virtual OpenDDS::Security::Utility {
public:
  virtual ~UtilityImpl();
  virtual void generate_random_bytes(void* ptr, size_t size);
  virtual void hmac(void* out, const void* in, size_t size, const std::string& password) const;
};

} // Security
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
