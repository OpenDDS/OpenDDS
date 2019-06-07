/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#ifndef DDS_DCPS_UTILITY_H
#define DDS_DCPS_UTILITY_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/SequenceIterator.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "TokenReader.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class DdsSecurity_Export Utility {
public:
  virtual ~Utility() {}
  virtual void generate_random_bytes(void* ptr, size_t size) = 0;
  virtual void hmac(void* out, void const* in, size_t size, const std::string& password) const = 0;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
