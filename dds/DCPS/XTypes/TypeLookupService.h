/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H
#define OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>

#include <ace/Thread_Mutex.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export TypeLookupService : public virtual DCPS::RcObject {
public:
  TypeLookupService();
  ~TypeLookupService();

  // For TypeAssignability
  const TypeObject& get_type_objects(const TypeIdentifier& type_id) const;
  void add(const TypeIdentifier& ti, const TypeObject& tobj);

  // For TypeLookup_getTypes
  void get_type_objects(const TypeIdentifierSeq& type_ids,
    TypeIdentifierTypeObjectPairSeq& types) const;
  void add_type_objects_to_cache(const TypeIdentifierTypeObjectPairSeq& types);

  void update_type_identifier_map(const TypeIdentifierPairSeq& tid_pairs);
  bool complete_to_minimal_type_object(const TypeObject& cto, TypeObject& mto) const;

  // For TypeLookup_getTypeDependencies
  bool get_type_dependencies(const TypeIdentifier& type_id,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void get_type_dependencies(const TypeIdentifierSeq& type_ids,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void add_type_dependencies(const TypeIdentifier& type_id,
    const TypeIdentifierWithSizeSeq& dependencies);

  // For adding local endpoint types
  void add(TypeMap::const_iterator begin, TypeMap::const_iterator end);

  bool type_object_in_cache(const TypeIdentifier& ti) const;
  bool extensibility(TypeFlag extensibility_mask, const TypeIdentifier& ti) const;

private:
  const TypeObject& get_type_objects_i(const TypeIdentifier& type_id) const;
  void get_type_dependencies_i(const TypeIdentifierSeq& type_ids,
    TypeIdentifierWithSizeSeq& dependencies) const;

  // Contains both minimal and complete type mapping.
  TypeMap type_map_;

  // For dependencies of local types
  typedef OPENDDS_MAP(TypeIdentifier, TypeIdentifierWithSizeSeq) TypeIdentifierWithSizeSeqMap;
  TypeIdentifierWithSizeSeqMap type_dependencies_map_;

  mutable ACE_Thread_Mutex mutex_;

  TypeObject to_empty_;

  // Mapping from complete to minimal TypeIdentifiers of dependencies of remote types.
  typedef OPENDDS_MAP(TypeIdentifier, TypeIdentifier) TypeIdentifierMap;
  TypeIdentifierMap complete_to_minimal_ti_map_;

  DCPS::String equivalence_hash_to_string(const EquivalenceHash& hash) const;
  bool get_minimal_type_identifier(const TypeIdentifier& ct, TypeIdentifier& mt) const;

  bool complete_to_minimal_struct(const CompleteStructType& ct, MinimalStructType& mt) const;
  bool complete_to_minimal_union(const CompleteUnionType& ct, MinimalUnionType& mt) const;
  bool complete_to_minimal_annotation(const CompleteAnnotationType& ct, MinimalAnnotationType& mt) const;
  bool complete_to_minimal_alias(const CompleteAliasType& ct, MinimalAliasType& mt) const;
  bool complete_to_minimal_sequence(const CompleteSequenceType& ct, MinimalSequenceType& mt) const;
  bool complete_to_minimal_array(const CompleteArrayType& ct, MinimalArrayType& mt) const;
  bool complete_to_minimal_map(const CompleteMapType& ct, MinimalMapType& mt) const;
  bool complete_to_minimal_enumerated(const CompleteEnumeratedType& ct, MinimalEnumeratedType& mt) const;
  bool complete_to_minimal_bitmask(const CompleteBitmaskType& ct, MinimalBitmaskType& mt) const;
  bool complete_to_minimal_bitset(const CompleteBitsetType& ct, MinimalBitsetType& mt) const;
};

typedef DCPS::RcHandle<TypeLookupService> TypeLookupService_rch;

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


#endif /* ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H */
