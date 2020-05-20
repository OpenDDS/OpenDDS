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
    bool assignable(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
    bool assignable_alias(const MinimalTypeObject& ta,
                          const MinimalTypeObject& tb) const;
    bool assignable_annotation(const MinimalTypeObject& ta,
                               const MinimalTypeObject& tb) const;
    bool assignable_annotation(const MinimalTypeObject& ta,
                               const TypeIdentifier& tb) const;
    bool assignable_struct(const MinimalTypeObject& ta,
                           const MinimalTypeObject& tb) const;
    bool assignable_struct(const MinimalTypeObject& ta,
                           const TypeIdentifier& tb) const;
    bool assignable_union(const MinimalTypeObject& ta,
                          const MinimalTypeObject& tb) const;
    bool assignable_union(const MinimalTypeObject& ta,
                          const TypeIdentifier& tb) const;
    bool assignable_bitset(const MinimalTypeObject& ta,
                           const MinimalTypeObject& tb) const;
    bool assignable_bitset(const MinimalTypeObject& ta,
                           const TypeIdentifier& tb) const;
    bool assignable_sequence(const MinimalTypeObject& ta,
                             const MinimalTypeObject& tb) const;
    bool assignable_sequence(const MinimalTypeObject& ta,
                             const TypeIdentifier& tb) const;
    bool assignable_array(const MinimalTypeObject& ta,
                          const MinimalTypeObject& tb) const;
    bool assignable_array(const MinimalTypeObject& ta,
                          const TypeIdentifier& tb) const;
    bool assignable_map(const MinimalTypeObject& ta,
                        const MinimalTypeObject& tb) const;
    bool assignable_map(const MinimalTypeObject& ta,
                        const TypeIdentifier& tb) const;
    bool assignable_enum(const MinimalTypeObject& ta,
                         const MinimalTypeObject& tb) const;
    bool assignable_enum(const MinimalTypeObject& ta,
                         const TypeIdentifier& tb) const;
    bool assignable_bitmask(const MinimalTypeObject& ta,
                            const MinimalTypeObject& tb) const;
    bool assignable_bitmask(const MinimalTypeObject& ta,
                            const TypeIdentifier& tb) const;
    bool assignable_extended(const MinimalTypeObject& ta,
                             const MinimalTypeObject& tb) const;

    bool assignable_primitive(const TypeIdentifier& ta,
                              const TypeIdentifier& tb) const;
    bool assignable_primitive(const TypeIdentifier& ta,
                              const MinimalTypeObject& tb) const;
    bool assignable_string(const TypeIdentifier& ta,
                           const TypeIdentifier& tb) const;
    bool assignable_string(const TypeIdentifier& ta,
                           const MinimalTypeObject& tb) const;
    bool assignable_plain_sequence(const TypeIdentifier& ta,
                                   const TypeIdentifier& tb) const;
    bool assignable_plain_sequence(const TypeIdentifier& ta,
                                   const MinimalTypeObject& tb) const;
    bool assignable_plain_array(const TypeIdentifier& ta,
                                const TypeIdentifier& tb) const;
    bool assignable_plain_array(const TypeIdentifier& ta,
                                const MinimalTypeObject& tb) const;
    bool assignable_plain_map(const TypeIdentifier& ta,
                              const TypeIdentifier& tb) const;
    bool assignable_plain_map(const TypeIdentifier& ta,
                              const MinimalTypeObject& tb) const;

    // General helpers
    bool strongly_assignable(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
    bool is_delimited(const TypeIdentifier& ti) const;

  };

} // namepace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TYPE_ASSIGNABILITY_H */
