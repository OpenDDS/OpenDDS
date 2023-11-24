/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_UTILITY_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_UTILITY_H

#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/RcObject.h>
#include <dds/Versioned_Namespace.h>

#include <string>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Dcps_Export Utility : public DCPS::RcObject {
public:
  virtual ~Utility() {}
  virtual void generate_random_bytes(void* ptr, size_t size) = 0;
  virtual void hmac(void* out, void const* in, size_t size, const std::string& password) const = 0;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
