/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_UTILS_H
#define OPENDDS_DCPS_XTYPES_UTILS_H

#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class TypeLookupService;


/**
 * Walk the TypeObject graph starting at top_level looking for all TypeIdentifiers
 * that match enum_type and replacing each of those with a new TypeIdentifier that
 * represents the orignal enum_type with certain enumerators (represented by the
 * values sequence) removed.  All newly-generated TypeObjects are inserted into
 * the type_map with their respective TypeIdentifiers.
 * The returned TypeIdentifier represents the modified top_level.
 */
OpenDDS_Dcps_Export
TypeIdentifier remove_enumerators(const TypeIdentifier& top_level,
                                  const TypeIdentifier& enum_type,
                                  const Sequence<DDS::Int32>& values,
                                  const TypeLookupService& lookup,
                                  TypeMap& type_map,
                                  TypeIdentifier* modified_enum = 0);

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
