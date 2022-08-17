/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H
#define OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H

#include "TypeObject.h"
#include "DynamicTypeMemberImpl.h"
#include "MemberDescriptorImpl.h"
#include "TypeDescriptorImpl.h"
#include "DynamicTypeImpl.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/GuidUtils.h>

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

  /// For TypeAssignability
  const TypeObject& get_type_object(const TypeIdentifier& type_id) const;
  void add(const TypeIdentifier& ti, const TypeObject& tobj);

  /// For TypeLookup_getTypes
  void get_type_objects(const TypeIdentifierSeq& type_ids,
    TypeIdentifierTypeObjectPairSeq& types) const;
  void add_type_objects_to_cache(const TypeIdentifierTypeObjectPairSeq& types);

  /// For converting between complete to minimal TypeObject of remote types
  ///@{
  void update_type_identifier_map(const TypeIdentifierPairSeq& tid_pairs);
  bool complete_to_minimal_type_object(const TypeObject& cto, TypeObject& mto) const;
  ///@}

#ifndef OPENDDS_SAFETY_PROFILE
  typedef OPENDDS_MAP(TypeIdentifier, DDS::DynamicType_var) DynamicTypeMap;
  typedef OPENDDS_MAP(DCPS::GUID_t, DynamicTypeMap) GuidTypeMap;
  DDS::DynamicType_ptr complete_to_dynamic(const CompleteTypeObject& cto, const DCPS::GUID_t& guid);
  void remove_guid_from_dynamic_map(const DCPS::GUID_t& guid);

  DDS::DynamicType_ptr type_identifier_to_dynamic(const TypeIdentifier& ti, const DCPS::GUID_t& guid);
#endif // OPENDDS_SAFETY_PROFILE

  /// For TypeLookup_getTypeDependencies
  bool get_type_dependencies(const TypeIdentifier& type_id,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void get_type_dependencies(const TypeIdentifierSeq& type_ids,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void add_type_dependencies(const TypeIdentifier& type_id,
    const TypeIdentifierWithSizeSeq& dependencies);

  /// For adding local endpoint types
  void add(TypeMap::const_iterator begin, TypeMap::const_iterator end);

  bool type_object_in_cache(const TypeIdentifier& ti) const;
  bool extensibility(TypeFlag extensibility_mask, const TypeIdentifier& ti) const;

  /// For caching and retrieving TypeInformation of remote endpoints
  void cache_type_info(const DDS::BuiltinTopicKey_t& key, const TypeInformation& type_info);
  const TypeInformation& get_type_info(const DDS::BuiltinTopicKey_t& key) const;

private:
  const TypeObject& get_type_object_i(const TypeIdentifier& type_id) const;
  void get_type_dependencies_i(const TypeIdentifierSeq& type_ids,
    TypeIdentifierWithSizeSeq& dependencies) const;

  /// Contains both minimal and complete type mapping.
  TypeMap type_map_;

  /// For dependencies of local types
  typedef OPENDDS_MAP(TypeIdentifier, TypeIdentifierWithSizeSeq) TypeIdentifierWithSizeSeqMap;
  TypeIdentifierWithSizeSeqMap type_dependencies_map_;

  mutable ACE_Thread_Mutex mutex_;

  TypeObject to_empty_;

  /// Mapping from complete to minimal TypeIdentifiers of dependencies of remote types.
  typedef OPENDDS_MAP(TypeIdentifier, TypeIdentifier) TypeIdentifierMap;
  TypeIdentifierMap complete_to_minimal_ti_map_;

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

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::MemberDescriptor* complete_struct_member_to_member_descriptor(const CompleteStructMember& cm, const DCPS::GUID_t& guid);
  DDS::MemberDescriptor* complete_union_member_to_member_descriptor(const CompleteUnionMember& cm, const DCPS::GUID_t& guid);
  DDS::MemberDescriptor* complete_annotation_member_to_member_descriptor(const CompleteAnnotationParameter& cm, const DCPS::GUID_t& guid);
  void complete_to_dynamic_i(DynamicTypeImpl* dt, const CompleteTypeObject& cto, const DCPS::GUID_t& guid);
  GuidTypeMap gt_map_;
#endif
  /// Map from BuiltinTopicKey_t of remote endpoint to its TypeInformation.
  typedef OPENDDS_MAP_CMP(DDS::BuiltinTopicKey_t, TypeInformation,
                          DCPS::BuiltinTopicKey_tKeyLessThan) TypeInformationMap;
  TypeInformationMap type_info_map_;
  TypeInformation type_info_empty_;
};

typedef DCPS::RcHandle<TypeLookupService> TypeLookupService_rch;

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


#endif /* ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H */
