/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPE_COMPATIBILITY_H
#define OPENDDS_DCPS_TYPE_COMPATIBILITY_H

#include "dds/DCPS/TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

  bool compatible(const TypeIdentifier& ta, const TypeIdentifier& tb);

} // namepace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TYPE_COMPATIBILITY_H */
