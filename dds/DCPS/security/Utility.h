/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#ifndef OPENDDS_DCPS_SECURITY_UTILITY_H
#define OPENDDS_DCPS_SECURITY_UTILITY_H

#include "OpenDDS_Security_Export.h"
#include "TokenReader.h"

#include <dds/DCPS/SequenceIterator.h>
#include <dds/DCPS/RcObject.h>
#include <dds/Versioned_Namespace.h>

#include <dds/DdsSecurityCoreC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Security_Export Utility : public DCPS::RcObject {
public:
  virtual ~Utility() {}
  virtual void generate_random_bytes(void* ptr, size_t size) = 0;
  virtual void hmac(void* out, void const* in, size_t size, const std::string& password) const = 0;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
