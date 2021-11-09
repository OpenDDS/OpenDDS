/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include <DCPS/DdsDcps_pch.h>

#include "TypeLookupService.h"

#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {

using namespace OpenDDS::XTypes;
void handle_tryconstruct_flags(MemberDescriptor& md, MemberFlag mf)
{
  if (mf & TRY_CONSTRUCT1) {
    md.try_construct_kind = (mf & TRY_CONSTRUCT2) ? TRIM : DISCARD;
  } else if (mf & TRY_CONSTRUCT2) {
    md.try_construct_kind = USE_DEFAULT;
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) handle_tryconstruct_flags -")
               ACE_TEXT(" Invalid TryConstruct Kind\n")));
  }
}

}

namespace OpenDDS {
namespace XTypes {

TypeLookupService::TypeLookupService()
{
  to_empty_.minimal.kind = TK_NONE;
  to_empty_.complete.kind = TK_NONE;

  type_info_empty_.minimal.typeid_with_size.type_id = TypeIdentifier(TK_NONE);
  type_info_empty_.complete.typeid_with_size.type_id = TypeIdentifier(TK_NONE);
}

TypeLookupService::~TypeLookupService()
{
}

void TypeLookupService::get_type_objects(const TypeIdentifierSeq& type_ids,
                                         TypeIdentifierTypeObjectPairSeq& types) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (unsigned i = 0; i < type_ids.length(); ++i) {
    TypeMap::const_iterator pos = type_map_.find(type_ids[i]);
    if (pos != type_map_.end()) {
      types.append(TypeIdentifierTypeObjectPair(pos->first, pos->second));
    }
  }
}

const TypeObject& TypeLookupService::get_type_object(const TypeIdentifier& type_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, to_empty_);
  return get_type_object_i(type_id);
}

const TypeObject& TypeLookupService::get_type_object_i(const TypeIdentifier& type_id) const
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
  for (unsigned i = 0; i < types.length(); ++i) {
    const TypeMap::iterator pos = type_map_.find(types[i].type_identifier);
    if (pos == type_map_.end()) {
      type_map_.insert(std::make_pair(types[i].type_identifier, types[i].type_object));
    }
  }
}

void TypeLookupService::add(TypeMap::const_iterator begin, TypeMap::const_iterator end)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  type_map_.insert(begin, end);
}

void TypeLookupService::add(const TypeIdentifier& ti, const TypeObject& tobj)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  TypeMap::const_iterator pos = type_map_.find(ti);
  if (pos == type_map_.end()) {
    type_map_.insert(std::make_pair(ti, tobj));
  }
}

void TypeLookupService::update_type_identifier_map(const TypeIdentifierPairSeq& tid_pairs)
{
  for (ACE_CDR::ULong i = 0; i < tid_pairs.length(); ++i) {
    const TypeIdentifierPair& pair = tid_pairs[i];
    complete_to_minimal_ti_map_.insert(std::make_pair(pair.type_identifier1, pair.type_identifier2));
  }
}

void TypeLookupService::cache_type_info(const DDS::BuiltinTopicKey_t& key,
                                        const TypeInformation& type_info)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (type_info_map_.find(key) == type_info_map_.end()) {
    type_info_map_.insert(std::make_pair(key, type_info));
  }
}

const TypeInformation& TypeLookupService::get_type_info(const DDS::BuiltinTopicKey_t& key) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, type_info_empty_);
  const TypeInformationMap::const_iterator it = type_info_map_.find(key);
  if (it != type_info_map_.end()) {
    return it->second;
  }
  return type_info_empty_;
}

bool TypeLookupService::get_minimal_type_identifier(const TypeIdentifier& ct, TypeIdentifier& mt) const
{
  if (ct.kind() == TK_NONE || is_fully_descriptive(ct)) {
    mt = ct;
    return true;
  }

  // Non-fully descriptive plain collection is a special case where we have to
  // get the minimal TypeIdentifier of the element type to create the TypeIdentifier
  // of the minimal collection type.
  if (is_plain_collection(ct)) {
    mt = ct;
    TypeIdentifier complete_elem_ti, minimal_elem_ti;
    switch (mt.kind()) {
    case TI_PLAIN_SEQUENCE_SMALL:
      mt.seq_sdefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.seq_sdefn().element_identifier;
      break;
    case TI_PLAIN_SEQUENCE_LARGE:
      mt.seq_ldefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.seq_ldefn().element_identifier;
      break;
    case TI_PLAIN_ARRAY_SMALL:
      mt.array_sdefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.array_sdefn().element_identifier;
      break;
    case TI_PLAIN_ARRAY_LARGE:
      mt.array_ldefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.array_ldefn().element_identifier;
      break;
    case TI_PLAIN_MAP_SMALL:
      mt.map_sdefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.map_sdefn().element_identifier;
      break;
    case TI_PLAIN_MAP_LARGE:
      mt.map_ldefn().header.equiv_kind = EK_MINIMAL;
      complete_elem_ti = *ct.map_ldefn().element_identifier;
      break;
    }
    get_minimal_type_identifier(complete_elem_ti, minimal_elem_ti);

    switch (mt.kind()) {
    case TI_PLAIN_SEQUENCE_SMALL:
      mt.seq_sdefn().element_identifier = minimal_elem_ti;
      break;
    case TI_PLAIN_SEQUENCE_LARGE:
      mt.seq_ldefn().element_identifier = minimal_elem_ti;
      break;
    case TI_PLAIN_ARRAY_SMALL:
      mt.array_sdefn().element_identifier = minimal_elem_ti;
      break;
    case TI_PLAIN_ARRAY_LARGE:
      mt.array_ldefn().element_identifier = minimal_elem_ti;
      break;
    case TI_PLAIN_MAP_SMALL:
      {
        mt.map_sdefn().element_identifier = minimal_elem_ti;
        TypeIdentifier minimal_key_ti;
        get_minimal_type_identifier(*ct.map_sdefn().key_identifier, minimal_key_ti);
        mt.map_sdefn().key_identifier = minimal_key_ti;
        break;
      }
    case TI_PLAIN_MAP_LARGE:
      {
        mt.map_ldefn().element_identifier = minimal_elem_ti;
        TypeIdentifier minimal_key_ti;
        get_minimal_type_identifier(*ct.map_ldefn().key_identifier, minimal_key_ti);
        mt.map_ldefn().key_identifier = minimal_key_ti;
        break;
      }
    }

    return true;
  }

  // Mapping for the remaining type kinds should be provided by the remote endpoint.
  const TypeIdentifierMap::const_iterator pos = complete_to_minimal_ti_map_.find(ct);
  if (pos == complete_to_minimal_ti_map_.end()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::get_minimal_type_identifier: ")
               ACE_TEXT("complete TypeIdentifier not found.\n")));
    if (ct.kind() == EK_COMPLETE) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::get_minimal_type_identifier: ")
                 ACE_TEXT(" Kind: EK_COMPLETE. Hash: (%C)\n"),
                 equivalence_hash_to_string(ct.equivalence_hash()).c_str()));
    } else if (ct.kind() == TI_STRONGLY_CONNECTED_COMPONENT) {
      const EquivalenceKind ek = ct.sc_component_id().sc_component_id.kind;
      if (ek == EK_MINIMAL) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::get_minimal_type_identifier: ")
                   ACE_TEXT("Expect EK_COMPLETE but received EK_MINIMAL.\n")));
      }
      const DCPS::String ek_str = ek == EK_COMPLETE ? "EK_COMPLETE" : "EK_MINIMAL";
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::get_minimal_type_identifier: ")
                 ACE_TEXT("Kind: TI_STRONGLY_CONNECTED_COMPONENT. ")
                 ACE_TEXT("Equivalence kind: %C. Hash: (%C). Scc length: %d. Scc index: %d\n"),
                 ek_str.c_str(),
                 equivalence_hash_to_string(ct.sc_component_id().sc_component_id.hash).c_str(),
                 ct.sc_component_id().scc_length,
                 ct.sc_component_id().scc_index));
    }

    mt = TypeIdentifier(TK_NONE);
    return false;
  }
  mt = pos->second;
  return true;
}

bool TypeLookupService::complete_to_minimal_struct(const CompleteStructType& ct,
                                                   MinimalStructType& mt) const
{
  mt.struct_flags = ct.struct_flags;
  if (!get_minimal_type_identifier(ct.header.base_type, mt.header.base_type)) {
    return false;
  }
  mt.member_seq.length(ct.member_seq.length());

  for (ACE_CDR::ULong i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common.member_id = ct.member_seq[i].common.member_id;
    mt.member_seq[i].common.member_flags = ct.member_seq[i].common.member_flags;
    if (!get_minimal_type_identifier(ct.member_seq[i].common.member_type_id,
                                     mt.member_seq[i].common.member_type_id)) {
      return false;
    }
    hash_member_name(mt.member_seq[i].detail.name_hash, ct.member_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_union(const CompleteUnionType& ct,
                                                  MinimalUnionType& mt) const
{
  mt.union_flags = ct.union_flags;
  mt.discriminator.common.member_flags = ct.discriminator.common.member_flags;
  if (!get_minimal_type_identifier(ct.discriminator.common.type_id,
                                   mt.discriminator.common.type_id)) {
    return false;
  }
  mt.member_seq.length(ct.member_seq.length());

  for (ACE_CDR::ULong i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common.member_id = ct.member_seq[i].common.member_id;
    mt.member_seq[i].common.member_flags = ct.member_seq[i].common.member_flags;
    if (!get_minimal_type_identifier(ct.member_seq[i].common.type_id,
                                     mt.member_seq[i].common.type_id)) {
      return false;
    }
    mt.member_seq[i].common.label_seq = ct.member_seq[i].common.label_seq;
    hash_member_name(mt.member_seq[i].detail.name_hash, ct.member_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_annotation(const CompleteAnnotationType& ct,
                                                       MinimalAnnotationType& mt) const
{
  mt.annotation_flag = ct.annotation_flag;
  mt.member_seq.length(ct.member_seq.length());

  for (ACE_CDR::ULong i = 0; i < ct.member_seq.length(); ++i) {
    mt.member_seq[i].common.member_flags = ct.member_seq[i].common.member_flags;
    if (!get_minimal_type_identifier(ct.member_seq[i].common.member_type_id,
                                     mt.member_seq[i].common.member_type_id)) {
      return false;
    }
    hash_member_name(mt.member_seq[i].name_hash, ct.member_seq[i].name);
    mt.member_seq[i].default_value = ct.member_seq[i].default_value;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_alias(const CompleteAliasType& ct,
                                                  MinimalAliasType& mt) const
{
  mt.alias_flags = ct.alias_flags;
  mt.body.common.related_flags = ct.body.common.related_flags;
  if (!get_minimal_type_identifier(ct.body.common.related_type,
                                   mt.body.common.related_type)) {
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_sequence(const CompleteSequenceType& ct,
                                                     MinimalSequenceType& mt) const
{
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.element.common.element_flags = ct.element.common.element_flags;
  if (!get_minimal_type_identifier(ct.element.common.type, mt.element.common.type)) {
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_array(const CompleteArrayType& ct,
                                                  MinimalArrayType& mt) const
{
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.element.common.element_flags = ct.element.common.element_flags;
  if (!get_minimal_type_identifier(ct.element.common.type, mt.element.common.type)) {
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_map(const CompleteMapType& ct,
                                                MinimalMapType& mt) const
{
  mt.collection_flag = ct.collection_flag;
  mt.header.common = ct.header.common;
  mt.key.common.element_flags = ct.key.common.element_flags;
  if (!get_minimal_type_identifier(ct.key.common.type, mt.key.common.type)) {
    return false;
  }
  mt.element.common.element_flags = ct.element.common.element_flags;
  if (!get_minimal_type_identifier(ct.element.common.type, mt.element.common.type)) {
    return false;
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_enumerated(const CompleteEnumeratedType& ct,
                                                       MinimalEnumeratedType& mt) const
{
  mt.enum_flags = ct.enum_flags;
  mt.header.common = ct.header.common;
  mt.literal_seq.length(ct.literal_seq.length());
  for (ACE_CDR::ULong i = 0; i < ct.literal_seq.length(); ++i) {
    mt.literal_seq[i].common = ct.literal_seq[i].common;
    hash_member_name(mt.literal_seq[i].detail.name_hash, ct.literal_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_bitmask(const CompleteBitmaskType& ct,
                                                    MinimalBitmaskType& mt) const
{
  mt.bitmask_flags = ct.bitmask_flags;
  mt.header.common = ct.header.common;
  mt.flag_seq.length(ct.flag_seq.length());
  for (ACE_CDR::ULong i = 0; i < ct.flag_seq.length(); ++i) {
    mt.flag_seq[i].common = ct.flag_seq[i].common;
    hash_member_name(mt.flag_seq[i].detail.name_hash, ct.flag_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_bitset(const CompleteBitsetType& ct,
                                                   MinimalBitsetType& mt) const
{
  mt.bitset_flags = ct.bitset_flags;
  mt.field_seq.length(ct.field_seq.length());
  for (ACE_CDR::ULong i = 0; i < ct.field_seq.length(); ++i) {
    mt.field_seq[i].common = ct.field_seq[i].common;
    hash_member_name(mt.field_seq[i].name_hash, ct.field_seq[i].detail.name);
  }
  return true;
}

bool TypeLookupService::complete_to_minimal_type_object(const TypeObject& cto, TypeObject& mto) const
{
  mto.kind = EK_MINIMAL;
  mto.minimal.kind = cto.complete.kind;

  switch (cto.complete.kind) {
  case TK_ALIAS:
    return complete_to_minimal_alias(cto.complete.alias_type, mto.minimal.alias_type);
  case TK_ANNOTATION:
    return complete_to_minimal_annotation(cto.complete.annotation_type, mto.minimal.annotation_type);
  case TK_STRUCTURE:
    return complete_to_minimal_struct(cto.complete.struct_type, mto.minimal.struct_type);
  case TK_UNION:
    return complete_to_minimal_union(cto.complete.union_type, mto.minimal.union_type);
  case TK_BITSET:
    return complete_to_minimal_bitset(cto.complete.bitset_type, mto.minimal.bitset_type);
  case TK_SEQUENCE:
    return complete_to_minimal_sequence(cto.complete.sequence_type, mto.minimal.sequence_type);
  case TK_ARRAY:
    return complete_to_minimal_array(cto.complete.array_type, mto.minimal.array_type);
  case TK_MAP:
    return complete_to_minimal_map(cto.complete.map_type, mto.minimal.map_type);
  case TK_ENUM:
    return complete_to_minimal_enumerated(cto.complete.enumerated_type, mto.minimal.enumerated_type);
  case TK_BITMASK:
    return complete_to_minimal_bitmask(cto.complete.bitmask_type, mto.minimal.bitmask_type);
  default:
    return false;
  }
}

bool TypeLookupService::complete_struct_member_to_member_descriptor(
  MemberDescriptor& md, const CompleteStructMember& cm, const DCPS::GUID_t& guid)
{
  md.name = cm.detail.name;
  if (cm.common.member_id > DCPS::Serializer::MEMBER_ID_MAX) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TypeLookupService::complete_struct_member_to_member_descriptor -")
               ACE_TEXT(" Member id exceeds maximum allowed value\n")));
    return false;
  }
  md.id = cm.common.member_id;
  md.type = type_identifier_to_dynamic(cm.common.member_type_id, guid);
  if (!md.type) {
    return false;
  }
  md.default_value = "";
  md.label.length(0);
  handle_tryconstruct_flags(md, cm.common.member_flags);
  md.is_key = cm.common.member_flags & IS_KEY;
  md.is_optional = cm.common.member_flags & IS_OPTIONAL;
  md.is_must_understand = cm.common.member_flags & IS_MUST_UNDERSTAND;
  md.is_shared = cm.common.member_flags & IS_EXTERNAL;
  md.is_default_label = false;
  return true;
}

bool TypeLookupService::complete_union_member_to_member_descriptor(
  MemberDescriptor& md, const CompleteUnionMember& cm, const DCPS::GUID_t& guid)
{
  md.name = cm.detail.name;
  if (cm.common.member_id > DCPS::Serializer::MEMBER_ID_MAX) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TypeLookupService::complete_union_member_to_member_descriptor -")
               ACE_TEXT(" Member id exceeds maximum allowed value\n")));
    return false;
  }
  md.id = cm.common.member_id;
  md.type = type_identifier_to_dynamic(cm.common.type_id, guid);
  if (!md.type) {
    return false;
  }
  md.default_value = "";
  md.label = cm.common.label_seq;
  handle_tryconstruct_flags(md, cm.common.member_flags);
  md.is_key = false;
  md.is_optional = false;
  md.is_must_understand = false;
  md.is_shared = cm.common.member_flags & IS_EXTERNAL;
  md.is_default_label = cm.common.member_flags & IS_DEFAULT;
  return true;
}

bool TypeLookupService::complete_annotation_member_to_member_descriptor(
  MemberDescriptor& md, const CompleteAnnotationParameter& cm, const DCPS::GUID_t& guid)
{
  md.name = cm.name;
  md.type = type_identifier_to_dynamic(cm.common.member_type_id, guid);
  if (!md.type) {
    return false;
  }
  md.default_value = "";
  md.label.length(0);
  md.try_construct_kind = DISCARD;
  md.is_key = false;
  md.is_optional = false;
  md.is_must_understand = false;
  md.is_shared = false;
  md.is_default_label = false;
  return true;
}

DynamicType_rch TypeLookupService::complete_to_dynamic(const CompleteTypeObject& cto, const DCPS::GUID_t& guid)
{
  DynamicType_rch dt = DCPS::make_rch<DynamicType>();
  if (!complete_to_dynamic_i(dt, cto, guid)) {
    return DynamicType_rch();
  }
  return dt;
}

bool TypeLookupService::complete_to_dynamic_i(DynamicType_rch& dt,
  const CompleteTypeObject& cto, const DCPS::GUID_t& guid)
{
  TypeDescriptor td;
  switch (cto.kind) {
  case TK_ALIAS:
    td.kind = TK_ALIAS;
    td.name = cto.alias_type.header.detail.type_name;
    td.bound.length(0);
    td.base_type = type_identifier_to_dynamic(cto.alias_type.body.common.related_type, guid);
    if (!td.base_type) {
      return false;
    }
    // The spec says that Alias DynamicTypes should have DynamicTypeMembers, but that leads to redundancy
    break;
  case TK_ENUM:
    td.kind = TK_ENUM;
    td.name = cto.enumerated_type.header.detail.type_name;
    td.bound.length(1);
    td.bound[0] = cto.enumerated_type.header.common.bit_bound;
    for (ACE_CDR::ULong i = 0; i < cto.enumerated_type.literal_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      MemberDescriptor md;
      md.name = cto.enumerated_type.literal_seq[i].detail.name;
      md.type = dt;
      md.is_default_label = cto.enumerated_type.literal_seq[i].common.flags & IS_DEFAULT;
      md.index = i;
      dtm->set_descriptor(md);
      dtm->set_parent(dt);
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_BITMASK:
    td.kind = TK_BITMASK;
    td.name = cto.bitmask_type.header.detail.type_name;
    td.bound.length(1);
    td.bound[0] = cto.bitmask_type.header.common.bit_bound;
    td.element_type = type_identifier_to_dynamic(TypeIdentifier(TK_BOOLEAN), guid);
    for (ACE_CDR::ULong i = 0; i < cto.bitmask_type.flag_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      MemberDescriptor md;
      md.name = cto.bitmask_type.flag_seq[i].detail.name;
      md.type = type_identifier_to_dynamic(TypeIdentifier(TK_BOOLEAN), guid);
      md.index = i;
      dtm->set_descriptor(md);
      dtm->set_parent(dt);
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_ANNOTATION:
    td.kind = TK_ANNOTATION;
    td.name = cto.annotation_type.header.annotation_name;
    td.bound.length(0);
    for (ACE_CDR::ULong i = 0; i < cto.annotation_type.member_seq.length(); ++i) {
      MemberDescriptor md;
      if (!complete_annotation_member_to_member_descriptor(md, cto.annotation_type.member_seq[i], guid)) {
        return false;
      }
      md.index = i;
      md.id = i;
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      dtm->set_descriptor(md);
      dtm->set_parent(dt);
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_STRUCTURE:
    td.kind = TK_STRUCTURE;
    td.name = cto.struct_type.header.detail.type_name;
    td.bound.length(0);
    td.base_type = type_identifier_to_dynamic(cto.struct_type.header.base_type, guid);
    if (!td.base_type) {
      return false;
    }
    if (cto.struct_type.struct_flags & IS_FINAL) {
      td.extensibility_kind = FINAL;
    } else if (cto.struct_type.struct_flags & IS_APPENDABLE) {
      td.extensibility_kind = APPENDABLE;
    } else if (cto.struct_type.struct_flags & IS_MUTABLE) {
      td.extensibility_kind = MUTABLE;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::complete_to_dynamic -")
                 ACE_TEXT(" Invalid extensibility kind in TK_STRUCTURE\n")));
      return false;
    }
    td.is_nested = cto.struct_type.struct_flags & IS_NESTED;
    for (ACE_CDR::ULong i = 0; i < cto.struct_type.member_seq.length(); ++i) {
      MemberDescriptor md;
      if (!complete_struct_member_to_member_descriptor(md, cto.struct_type.member_seq[i], guid)) {
        return false;
      }
      md.index = i;
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      dtm->set_descriptor(md);
      dtm->set_parent(dt);
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_UNION:
    td.kind = TK_UNION;
    td.name = cto.union_type.header.detail.type_name;
    td.bound.length(0);
    td.discriminator_type = type_identifier_to_dynamic(cto.union_type.discriminator.common.type_id, guid);
    if (!td.discriminator_type) {
      return false;
    }
    if (cto.union_type.union_flags & IS_FINAL) {
      td.extensibility_kind = FINAL;
    } else if (cto.union_type.union_flags & IS_APPENDABLE) {
      td.extensibility_kind = APPENDABLE;
    } else if (cto.union_type.union_flags & IS_MUTABLE) {
      td.extensibility_kind = MUTABLE;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::complete_to_dynamic -")
                 ACE_TEXT(" Invalid extensibility kind in TK_UNION\n")));
      return false;
    }
    td.is_nested = cto.union_type.union_flags & IS_NESTED;
    for (ACE_CDR::ULong i = 0; i < cto.union_type.member_seq.length(); ++i) {
      MemberDescriptor md;
      if (!complete_union_member_to_member_descriptor(md, cto.union_type.member_seq[i], guid)) {
        return false;
      }
      md.index = i;
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      dtm->set_descriptor(md);
      dtm->set_parent(dt);
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_BITSET:
    td.kind = TK_BITSET;
    td.name = cto.bitset_type.header.detail.type_name;
    td.bound.length(0);
    break;
  case TK_SEQUENCE:
    td.kind = TK_SEQUENCE;
    if (cto.sequence_type.header.detail.present) {
      td.name = cto.sequence_type.header.detail.value.type_name;
    } else {
      td.name = "";
    }
    td.bound.length(1);
    td.bound[0] = cto.sequence_type.header.common.bound;
    td.element_type = type_identifier_to_dynamic(cto.sequence_type.element.common.type, guid);
    if (!td.element_type) {
      return false;
    }
    break;
  case TK_ARRAY:
    td.kind = TK_ARRAY;
    td.name = cto.array_type.header.detail.type_name;
    td.bound = cto.array_type.header.common.bound_seq;
    td.element_type = type_identifier_to_dynamic(cto.array_type.element.common.type, guid);
    if (!td.element_type) {
      return false;
    }
    break;
  case TK_MAP:
    td.kind = TK_MAP;
    if (cto.map_type.header.detail.present) {
      td.name = cto.map_type.header.detail.value.type_name;
    } else {
      td.name = "";
    }
    td.bound.length(1);
    td.bound[0] = cto.map_type.header.common.bound;
    td.element_type = type_identifier_to_dynamic(cto.map_type.element.common.type, guid);
    if (!td.element_type) {
      return false;
    }
    td.key_element_type = type_identifier_to_dynamic(cto.map_type.key.common.type, guid);
    if (!td.key_element_type) {
      return false;
    }
    break;
  }
  dt->set_descriptor(td);
  return true;
}

DynamicType_rch TypeLookupService::type_identifier_to_dynamic(const TypeIdentifier& ti,
  const DCPS::GUID_t& guid)
{
  if (ti.kind() == TK_NONE || ti.kind() == EK_MINIMAL) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TypeLookupService::type_identifier_to_dynamic -")
                 ACE_TEXT(" Encountered %C: returning nil DynamicType\n"),
                 ti.kind() == TK_NONE ? "TK_NONE" : "EK_MINIMAL"));
    }
    return DynamicType_rch();
  }

  DynamicType_rch dt = DCPS::make_rch<DynamicType>();
  TypeDescriptor td;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const GuidTypeMap::iterator guid_found = gt_map_.find(guid);
    if (guid_found != gt_map_.end()) {
      const DynamicTypeMap::const_iterator ti_found = guid_found->second.find(ti);
      if (ti_found != guid_found->second.end()) {
        return ti_found->second;
      } else {
        guid_found->second.insert(std::make_pair(ti, dt));
      }
    } else {
      DynamicTypeMap dt_map;
      dt_map.insert(std::make_pair(ti, dt));
      gt_map_.insert(std::make_pair(guid, dt_map));
    }
  }

  switch (ti.kind()) {
  case TK_BOOLEAN:
    td.kind = TK_BOOLEAN;
    td.name = "Boolean";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_BYTE:
    td.kind = TK_BYTE;
    td.name = "Byte";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_INT16:
    td.kind = TK_INT16;
    td.name = "Int16";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_INT32:
    td.kind = TK_INT32;
    td.name = "Int32";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_INT64:
    td.kind = TK_INT64;
    td.name = "Int64";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_UINT16:
    td.kind = TK_UINT16;
    td.name = "UInt16";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_UINT32:
    td.kind = TK_UINT32;
    td.name = "UInt32";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_UINT64:
    td.kind = TK_UINT64;
    td.name = "UInt64";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_FLOAT32:
    td.kind = TK_FLOAT32;
    td.name = "Float32";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_FLOAT64:
    td.kind = TK_FLOAT64;
    td.name = "Float64";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_FLOAT128:
    td.kind = TK_FLOAT128;
    td.name = "Float128";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_INT8:
    td.kind = TK_INT8;
    td.name = "Int8";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_UINT8:
    td.kind = TK_UINT8;
    td.name = "UInt8";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_CHAR8:
    td.kind = TK_CHAR8;
    td.name = "Char8";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TK_CHAR16:
    td.kind = TK_CHAR16;
    td.name = "Char16";
    td.bound.length(0);
    dt->set_descriptor(td);
    break;
  case TI_STRING8_SMALL:
    {
      td.kind = TK_STRING8;
      td.name = "String8Small";
      td.bound.length(1);
      td.bound[0] = ti.string_sdefn().bound;
      td.element_type = type_identifier_to_dynamic(TypeIdentifier(TK_CHAR8), guid);
      dt->set_descriptor(td);
      break;
    }
  case TI_STRING8_LARGE:
    {
      td.kind = TK_STRING8;
      td.name = "String8Large";
      td.bound.length(1);
      td.bound[0] = ti.string_ldefn().bound;
      td.element_type = type_identifier_to_dynamic(TypeIdentifier(TK_CHAR8), guid);
      dt->set_descriptor(td);
      break;
    }
  case TI_STRING16_SMALL:
    {
      td.kind = TK_STRING16;
      td.name = "WString16Small";
      td.bound.length(1);
      td.bound[0] = ti.string_sdefn().bound;
      td.element_type = type_identifier_to_dynamic(TypeIdentifier(TK_CHAR16), guid);
      dt->set_descriptor(td);
      break;
    }
  case TI_STRING16_LARGE:
    {
      td.kind = TK_STRING16;
      td.name = "WString16Large";
      td.bound.length(1);
      td.bound[0] = ti.string_ldefn().bound;
      td.element_type = type_identifier_to_dynamic(TypeIdentifier(TK_CHAR16), guid);
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_SEQUENCE_SMALL:
    {
      td.kind = TK_SEQUENCE;
      td.name = "SequenceSmall";
      td.bound.length(1);
      td.bound[0] = ti.seq_sdefn().bound;
      td.element_type = type_identifier_to_dynamic(*ti.seq_sdefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_SEQUENCE_LARGE:
    {
      td.kind = TK_SEQUENCE;
      td.name = "SequenceLarge";
      td.bound.length(1);
      td.bound[0] = ti.seq_ldefn().bound;
      td.element_type = type_identifier_to_dynamic(*ti.seq_ldefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_ARRAY_SMALL:
    {
      td.kind = TK_ARRAY;
      td.name = "ArraySmall";
      td.bound.length(ti.array_sdefn().array_bound_seq.length());
      for (ACE_CDR::ULong i = 0; i< td.bound.length(); ++i) {
        td.bound[i] = ti.array_sdefn().array_bound_seq[i];
      }
      td.element_type = type_identifier_to_dynamic(*ti.array_sdefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_ARRAY_LARGE:
    {
      td.kind = TK_ARRAY;
      td.name = "ArrayLarge";
      td.bound = ti.array_ldefn().array_bound_seq;
      td.element_type = type_identifier_to_dynamic(*ti.array_ldefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_MAP_SMALL:
    {
      td.kind = TK_MAP;
      td.name = "MapSmall";
      td.bound.length(1);
      td.bound[0] = ti.map_sdefn().bound;
      td.element_type = type_identifier_to_dynamic(*ti.map_sdefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      td.key_element_type = type_identifier_to_dynamic(*ti.map_sdefn().key_identifier, guid);
      if (!td.key_element_type) {
        return td.key_element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_PLAIN_MAP_LARGE:
    {
      td.kind = TK_MAP;
      td.name = "MapLarge";
      td.bound.length(1);
      td.bound[0] = ti.map_ldefn().bound;
      td.element_type = type_identifier_to_dynamic(*ti.map_ldefn().element_identifier, guid);
      if (!td.element_type) {
        return td.element_type;
      }
      td.key_element_type = type_identifier_to_dynamic(*ti.map_ldefn().key_identifier, guid);
      if (!td.key_element_type) {
        return td.key_element_type;
      }
      dt->set_descriptor(td);
      break;
    }
  case TI_STRONGLY_CONNECTED_COMPONENT:
    if (ti.sc_component_id().sc_component_id.kind == EK_MINIMAL) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TypeLookupService::type_identifier_to_dynamic -")
                   ACE_TEXT(" Strongly connected component has kind EK_MINIMAL\n")));
      }
      return DynamicType_rch();
    }
  case EK_COMPLETE:
    {
      const TypeObject& to = get_type_object_i(ti);
      if (to.complete.kind == TK_NONE) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::type_identifier_to_dynamic -")
                   ACE_TEXT(" get_type_object_i returned TK_NONE\n")));
        return DynamicType_rch();
      }
      if (!complete_to_dynamic_i(dt, to.complete, guid)) {
        return DynamicType_rch();
      }
      break;
    }
  }
  return dt;
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
  const TypeObject& to = get_type_object_i(type_id);
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
    const TypeObject& dep_to = get_type_object_i(dependencies[i].type_id);
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

void TypeLookupService::remove_guid_from_dynamic_map(DCPS::GUID_t guid)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  const GuidTypeMap::iterator g_found = gt_map_.find(guid);
  if (g_found != gt_map_.end()) {
    gt_map_.erase(g_found);
    if(DCPS::DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TypeLookupService::remove_guid_from_dynamic_map: ",
        "Alerted to removal of %s, removing GUID from GuidTypeMap.\n", DCPS::to_string(guid).c_str()));
    }
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
