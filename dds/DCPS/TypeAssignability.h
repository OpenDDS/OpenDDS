/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPE_ASSIGNABILITY_H
#define OPENDDS_DCPS_TYPE_ASSIGNABILITY_H

#include "dds/DCPS/TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

  class TypeAssignability {
  public:
    bool assignable(const TypeObject& ta, const TypeObject& tb) const;

  private:
    bool assignable_struct(const TypeObject& ta, const TypeObject& tb) const;
    bool assignable_union(const TypeObject& ta, const TypeObject& tb) const;
    bool assignable_primitive(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
    bool assignable_string(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  };

} // namepace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TYPE_ASSIGNABILITY_H */
