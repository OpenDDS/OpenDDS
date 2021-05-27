/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "TypeLookupService.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

TypeLookupService::TypeLookupService()
{
  to_empty_.minimal.kind = TK_NONE;
  to_empty_.complete.kind = TK_NONE;
}

TypeLookupService::~TypeLookupService()
{
}

void TypeLookupService::get_type_objects(const TypeIdentifierSeq& type_ids,
                                         TypeIdentifierTypeObjectPairSeq& types) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (CORBA::ULong i = 0; i < type_ids.length(); ++i) {
    TypeMap::const_iterator pos = type_map_.find(type_ids[i]);
    if (pos != type_map_.end()) {
      types.append(TypeIdentifierTypeObjectPair(pos->first, pos->second));
    }
  }
}

const TypeObject& TypeLookupService::get_type_objects(const TypeIdentifier& type_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, to_empty_);
  return get_type_objects_i(type_id);
}

const TypeObject& TypeLookupService::get_type_objects_i(const TypeIdentifier& type_id) const
{
  const TypeMap::const_iterator pos = type_map_.find(type_id);
  if (pos != type_map_.end()) {
    return pos->second;
  }
  return to_empty_;
}

bool TypeLookupService::get_type_dependencies(const TypeIdentifier& type_id,
  TypeIdentifierWithSizeSeq& dependencies) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  const TypeIdentifierWithSizeSeqMap::const_iterator it = type_dependencies_map_.find(type_id);
  if (it != type_dependencies_map_.end()) {
    dependencies = it->second;
    return true;
  }
  return false;
}

void TypeLookupService::get_type_dependencies(const TypeIdentifierSeq& type_ids,
  TypeIdentifierWithSizeSeq& dependencies) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  get_type_dependencies_i(type_ids, dependencies);
}

void TypeLookupService::get_type_dependencies_i(const TypeIdentifierSeq& type_ids,
  TypeIdentifierWithSizeSeq& dependencies) const
{
  OPENDDS_SET(TypeIdentifier) tmp;
  for (unsigned i = 0; i < type_ids.length(); ++i) {
    const TypeIdentifierWithSizeSeqMap::const_iterator it = type_dependencies_map_.find(type_ids[i]);
    if (it != type_dependencies_map_.end()) {
      for (unsigned j = 0; j < it->second.length(); ++j) {
        tmp.insert(it->second[j].type_id);
      }
    }
  }

  // All dependent TypeIdentifiers are expected to have an entry in the TypeObject cache.
  dependencies.length(static_cast<unsigned>(tmp.size()));
  OPENDDS_SET(TypeIdentifier)::const_iterator iter = tmp.begin();
  for (unsigned i = 0; iter != tmp.end(); ++i, ++iter) {
    const TypeMap::const_iterator tobj_it = type_map_.find(*iter);
    if (tobj_it != type_map_.end()) {
      dependencies[i].type_id = *iter;
      const size_t sz = DCPS::serialized_size(get_typeobject_encoding(), tobj_it->second);
      dependencies[i].typeobject_serialized_size = static_cast<unsigned>(sz);
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifierTypeObjectPairSeq& types)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (ACE_UINT32 i = 0; i < types.length(); ++i) {
    const TypeMap::iterator pos = type_map_.find(types[i].type_identifier);
    if (pos == type_map_.end()) {
      type_map_.insert(std::make_pair(types[i].type_identifier, types[i].type_object));
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  // TODO: This populates the map N times instead of 1.
  const XTypes::TypeMap& minimal_type_map = typesupport.getMinimalTypeMap();
  const XTypes::TypeMap& complete_type_map = typesupport.getCompleteTypeMap();
  const XTypes::TypeIdentifierMap& type_identifier_map = typesupport.getTypeIdentifierMap();
  type_map_.insert(minimal_type_map.begin(), minimal_type_map.end());
  type_map_.insert(complete_type_map.begin(), complete_type_map.end());
  type_identifier_map_.insert(type_identifier_map.begin(), type_identifier_map.end());
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  TypeMap::const_iterator pos = type_map_.find(ti);
  if (pos == type_map_.end()) {
    type_map_.insert(std::make_pair(ti, tobj));
  }
}

void TypeLookupService::add_type_identifiers_to_cache(const TypeIdentifierPairSeq& pair)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (ACE_UINT32 i = 0; i < pair.length(); ++i) {
    //type_identifier1 is assumed to be the complete type identifier
    const TypeIdentifierMap::iterator pos = type_identifier_map_.find(pair[i].type_identifier1);
    if (pos == type_identifier_map_.end()) {
      type_identifier_map_.insert(std::make_pair(pair[i].type_identifier1, pair[i].type_identifier2));
    }
  }
}

bool TypeLookupService::get_minimal_type_identifier(const TypeIdentifier& cti, TypeIdentifier& mti)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  const TypeIdentifierMap::const_iterator it = type_identifier_map_.find(cti);
  if (it != type_identifier_map_.end()) {
    mti = it->second;
    return true;
  }
  ACE_ERROR((LM_ERROR, ACE_TEXT("get_minimal_type_identifier(): Unable to convert complete type identifier to minimal\n")));
  return false;
}

bool TypeLookupService::complete_to_minimal_struct(const CompleteStructType& ct, MinimalStructType& mt) {
  mt.struct_flags = ct.struct_flags;
  if (!get_minimal_type_identifier(ct.header.base_type, mt.header.base_type)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("complete_to_minimal_union(): Failed to get minimal type identifier\n")));
    return false;
  }
  mt.member_seq.length(ct.member_seq.length());
  for(unsigned long i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common = ct.member_seq[i].common;
    hash_member_name(mt.member_seq[i].detail.name_hash, ct.member_seq[i].detail.name);
  }
  return true;
}

 bool TypeLookupService::complete_to_minimal_union(const CompleteUnionType& ct, MinimalUnionType& mt) {
  mt.union_flags = ct.union_flags;
  mt.discriminator.common = ct.discriminator.common;
  if (!get_minimal_type_identifier(ct.discriminator.common.type_id, mt.discriminator.common.type_id)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("complete_to_minimal_union(): Failed to get minimal type identifier\n")));
    return false;
  }
  mt.member_seq.length(ct.member_seq.length());
  for(unsigned long i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common = ct.member_seq[i].common;
    hash_member_name(mt.member_seq[i].detail.name_hash, ct.member_seq[i].detail.name);
  }
  return true;
}

 bool TypeLookupService::complete_to_minimal_annotation(const CompleteAnnotationType& ct, MinimalAnnotationType& mt) {
  mt.annotation_flag = ct.annotation_flag;
  mt.member_seq.length(ct.member_seq.length());
  for(unsigned long i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common = ct.member_seq[i].common;
    hash_member_name(mt.member_seq[i].name_hash, ct.member_seq[i].name);
    mt.member_seq[i].default_value = ct.member_seq[i].default_value;
  }
  return true;
}

 bool TypeLookupService::complete_to_minimal_alias(const CompleteAliasType& ct, MinimalAliasType& mt) {
  mt.alias_flags = ct.alias_flags;
  mt.body.common = ct.body.common;
  return true;
}

bool TypeLookupService::complete_to_minimal_sequence(const CompleteSequenceType& ct, MinimalSequenceType& mt) {
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.element.common = ct.element.common;
  if(!get_minimal_type_identifier(ct.element.common.type, mt.element.common.type)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("complete_to_minimal_union(): Failed to get minimal type identifier\n")));
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_array(const CompleteArrayType& ct, MinimalArrayType& mt) {
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.element.common = ct.element.common;
  if (!get_minimal_type_identifier(ct.element.common.type, mt.element.common.type)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("complete_to_minimal_union(): Failed to get minimal type identifier\n")));
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_map(const CompleteMapType& ct, MinimalMapType& mt) {
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.key.common = ct.key.common;
  mt.element.common = ct.element.common;
  return true;
}

bool TypeLookupService::complete_to_minimal_enumerated(const CompleteEnumeratedType& ct, MinimalEnumeratedType& mt) {
  mt.header.common = ct.header.common;
  mt.literal_seq.length(ct.literal_seq.length());
  for(unsigned long i = 0; i < ct.literal_seq.length(); ++i) {
    mt.literal_seq[i].common = ct.literal_seq[i].common;
    hash_member_name(mt.literal_seq[i].detail.name_hash, ct.literal_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_bitmask(const CompleteBitmaskType& ct, MinimalBitmaskType& mt) {
  mt.header.common = ct.header.common;
  mt.flag_seq.length(ct.flag_seq.length());
  for(unsigned long i = 0; i < ct.flag_seq.length(); ++i) {
    mt.flag_seq[i].common = ct.flag_seq[i].common;
    hash_member_name(mt.flag_seq[i].detail.name_hash, ct.flag_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_bitset(const CompleteBitsetType& ct, MinimalBitsetType& mt) {
  mt.field_seq.length(ct.field_seq.length());
  for(unsigned long i = 0; i < ct.field_seq.length(); ++i) {
    mt.field_seq[i].common = ct.field_seq[i].common;
    hash_member_name(mt.field_seq[i].name_hash, ct.field_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_typeobject(const TypeObject& cto, TypeObject& mto) {
  mto.kind = EK_MINIMAL;
  bool retval = false;
  mto.minimal.kind = cto.complete.kind;
  switch (cto.complete.kind) {
  case TK_ALIAS:
    retval = complete_to_minimal_alias(cto.complete.alias_type, mto.minimal.alias_type);
  case TK_ANNOTATION:
    retval = complete_to_minimal_annotation(cto.complete.annotation_type, mto.minimal.annotation_type);
  case TK_STRUCTURE:
    retval = complete_to_minimal_struct(cto.complete.struct_type, mto.minimal.struct_type);
  case TK_UNION:
    retval = complete_to_minimal_union(cto.complete.union_type, mto.minimal.union_type);
  case TK_BITSET:
    retval = complete_to_minimal_bitset(cto.complete.bitset_type, mto.minimal.bitset_type);
  case TK_SEQUENCE:
    retval = complete_to_minimal_sequence(cto.complete.sequence_type, mto.minimal.sequence_type);
  case TK_ARRAY:
    retval = complete_to_minimal_array(cto.complete.array_type, mto.minimal.array_type);
  case TK_MAP:
    retval = complete_to_minimal_map(cto.complete.map_type, mto.minimal.map_type);
  case TK_ENUM:
    retval = complete_to_minimal_enumerated(cto.complete.enumerated_type, mto.minimal.enumerated_type);
  case TK_BITMASK:
    retval = complete_to_minimal_bitmask(cto.complete.bitmask_type, mto.minimal.bitmask_type);
  }
  return retval;
}

void TypeLookupService::add_type_dependencies(const TypeIdentifier& type_id,
  const TypeIdentifierWithSizeSeq& dependencies)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (type_dependencies_map_.find(type_id) == type_dependencies_map_.end()) {
    type_dependencies_map_.insert(std::make_pair(type_id, dependencies));
  }
}

bool TypeLookupService::type_object_in_cache(const TypeIdentifier& ti) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return type_map_.find(ti) != type_map_.end();
}

bool TypeLookupService::extensibility(TypeFlag extensibility_mask, const TypeIdentifier& type_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  bool result = false;
  const TypeObject& to = get_type_objects_i(type_id);
  TypeKind tk = to.kind == EK_MINIMAL ? to.minimal.kind : to.complete.kind;

  if (TK_UNION == tk) {
    result = to.kind == EK_MINIMAL ?
      (to.minimal.union_type.union_flags & extensibility_mask) :
      (to.complete.union_type.union_flags & extensibility_mask);
  } else if (TK_STRUCTURE == tk) {
    result = to.kind == EK_MINIMAL ?
      (to.minimal.struct_type.struct_flags & extensibility_mask) :
      (to.complete.struct_type.struct_flags & extensibility_mask);
  }

  if (result) {
    return true;
  }

  TypeIdentifierWithSizeSeq dependencies;
  TypeIdentifierSeq type_ids;
  type_ids.append(type_id);
  get_type_dependencies_i(type_ids, dependencies);

  for (unsigned i = 0; i < dependencies.length(); ++i) {
    const TypeObject& dep_to = get_type_objects_i(dependencies[i].type_id);
    tk = dep_to.kind == EK_MINIMAL ? dep_to.minimal.kind : dep_to.complete.kind;

    if (TK_UNION == tk) {
      result = dep_to.kind == EK_MINIMAL ?
        (dep_to.minimal.union_type.union_flags & extensibility_mask) :
        (dep_to.complete.union_type.union_flags & extensibility_mask);
    } else if (TK_STRUCTURE == tk) {
      result = dep_to.kind == EK_MINIMAL ?
        (dep_to.minimal.struct_type.struct_flags & extensibility_mask) :
        (dep_to.complete.struct_type.struct_flags & extensibility_mask);
    }
    if (result) {
      return true;
    }
  }
  return false;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
