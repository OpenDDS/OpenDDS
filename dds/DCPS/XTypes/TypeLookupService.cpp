/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "TypeLookupService.h"

#include <sstream>

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
  for (unsigned i = 0; i < type_ids.length(); ++i) {
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

DCPS::String TypeLookupService::equivalence_hash_to_string(const EquivalenceHash& hash) const
{
  std::ostringstream out;
  out << "(";
  for (unsigned i = 0; i < sizeof(EquivalenceHash); ++i) {
    out << hash[i];
    if (i < sizeof(EquivalenceHash) - 1) {
      out << ", ";
    } else {
      out << ")";
    }
  }
  return out.str().c_str();
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("Kind: EK_COMPLETE. Hash: %C\n"),
                 equivalence_hash_to_string(ct.equivalence_hash()).c_str()));
    } else if (ct.kind() == TI_STRONGLY_CONNECTED_COMPONENT) {
      const EquivalenceKind ek = ct.sc_component_id().sc_component_id.kind;
      if (ek == EK_MINIMAL) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("Expect EK_COMPLETE but received EK_MINIMAL.\n")));
      }
      const DCPS::String ek_str = ek == EK_COMPLETE ? "EK_COMPLETE" : "EK_MINIMAL";
      ACE_ERROR((LM_ERROR, ACE_TEXT("Kind: TI_STRONGLY_CONNECTED_COMPONENT. ")
                 ACE_TEXT("Equivalence kind: %C. Hash: %C. Scc length: %d. Scc index: %d\n"),
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

void TypeLookupService::handle_tryconstruct_flags(MemberDescriptor_rch& md, MemberFlag mf)
{
  if (mf & TRY_CONSTRUCT1) {
    if (mf & TRY_CONSTRUCT2) {
      md->try_construct_kind = TRIM;
    } else {
      md->try_construct_kind = DISCARD;
    }
  } else {
    if (mf & TRY_CONSTRUCT2) {
      md->try_construct_kind = USE_DEFAULT;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TypeLookupService::handle_tryconstruct_flags -")
                 ACE_TEXT(" Invalid TryConstruct Kind\n")));
    }
  }
}

void TypeLookupService::complete_struct_member_to_member_descriptor(MemberDescriptor_rch& md,
  const CompleteStructMember& cm, DynamicTypeMap& dt_map)
{
  md->name = cm.detail.name;
  md->id = cm.common.member_id;
  type_identifier_to_dynamic(md->type, cm.common.member_type_id, dt_map);
  md->default_value = "";
  md->label.length(0);
  handle_tryconstruct_flags(md, cm.common.member_flags);
  md->is_key = cm.common.member_flags & IS_KEY;
  md->is_optional = cm.common.member_flags & IS_OPTIONAL;
  md->is_must_understand = cm.common.member_flags & IS_MUST_UNDERSTAND;
  md->is_shared = cm.common.member_flags & IS_EXTERNAL;
  md->is_default_label = false;
}

void TypeLookupService::complete_union_member_to_member_descriptor(MemberDescriptor_rch& md,
  const CompleteUnionMember& cm, DynamicTypeMap& dt_map)
{
  md->name = cm.detail.name;
  md->id = cm.common.member_id;
  type_identifier_to_dynamic(md->type, cm.common.type_id, dt_map);
  md->default_value = "";
  md->label = cm.common.label_seq;
  handle_tryconstruct_flags(md, cm.common.member_flags);
  md->is_key = false;
  md->is_optional = false;
  md->is_must_understand = false;
  md->is_shared = cm.common.member_flags & IS_EXTERNAL;
  md->is_default_label = cm.common.member_flags & IS_DEFAULT;
}

void TypeLookupService::complete_annotation_member_to_member_descriptor(MemberDescriptor_rch& md,
  const CompleteAnnotationParameter& cm, DynamicTypeMap& dt_map)
{
  md->name = cm.name;
  type_identifier_to_dynamic(md->type, cm.common.member_type_id, dt_map);
  md->default_value = "";
  md->label.length(0);
  md->try_construct_kind = DISCARD;
  md->is_key = false;
  md->is_optional = false;
  md->is_must_understand = false;
  md->is_shared = false;
  md->is_default_label = false;
}

void TypeLookupService::complete_to_dynamic(DynamicType_rch& dt,
  const CompleteTypeObject& cto)
  {
    DynamicType_rch dt_instantiation = DCPS::make_rch<DynamicType>();
    dt = dt_instantiation;
    DynamicTypeMap dt_map;
    complete_to_dynamic_i(dt, cto, dt_map);
  }

void TypeLookupService::complete_to_dynamic_i(DynamicType_rch& dt,
  const CompleteTypeObject& cto, DynamicTypeMap& dt_map)
{
  switch (cto.kind) {
  // Constructed/Named types
  case TK_ALIAS:
    dt->get_descriptor()->kind = TK_ALIAS;
    dt->get_descriptor()->name = cto.alias_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(0);
    type_identifier_to_dynamic(dt->get_descriptor()->base_type, cto.alias_type.body.common.related_type, dt_map);
    for (ACE_CDR::ULong i = 0; i < dt->get_descriptor()->base_type->get_member_count(); ++i) {
      DynamicTypeMember_rch dtm;
      dt->get_descriptor()->base_type->get_member_by_index(dtm, i);
      dtm->get_parent()= dt;
      dt->insert_dynamic_member(dtm);
    }
    break;
  // Enumerated TKs
  case TK_ENUM:
    dt->get_descriptor()->kind = TK_ENUM;
    dt->get_descriptor()->name = cto.enumerated_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(0);
    for (ACE_CDR::ULong i = 0; i < cto.enumerated_type.literal_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      dtm->get_descriptor()->name = cto.enumerated_type.literal_seq[i].detail.name;
      dtm->get_descriptor()->type = dt;
      dtm->get_descriptor()->is_default_label = (cto.enumerated_type.literal_seq[i].common.flags & IS_DEFAULT);
      dtm->get_descriptor()->index = i;
      dtm->get_parent() = dt;
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_BITMASK: {
    dt->get_descriptor()->kind = TK_BITMASK;
    dt->get_descriptor()->name = cto.bitmask_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(1);
    dt->get_descriptor()->bound[0] = cto.bitmask_type.header.common.bit_bound;
    type_identifier_to_dynamic(dt->get_descriptor()->element_type, TypeIdentifier(TK_BOOLEAN), dt_map);
    for (ACE_CDR::ULong i = 0; i < cto.bitmask_type.flag_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      dtm->get_descriptor()->name = cto.bitmask_type.flag_seq[i].detail.name;
      type_identifier_to_dynamic(dtm->get_descriptor()->type, TypeIdentifier(TK_BOOLEAN), dt_map);
      dtm->get_descriptor()->index = i;
      dtm->get_parent() = dt;
      dt->insert_dynamic_member(dtm);
    }
  }
  break;
  // Structured TKs
  case TK_ANNOTATION:
    dt->get_descriptor()->kind = TK_ANNOTATION;
    dt->get_descriptor()->name = cto.annotation_type.header.annotation_name;
    dt->get_descriptor()->bound.length(0);
    for (ACE_CDR::ULong i = 0; i < cto.annotation_type.member_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      complete_annotation_member_to_member_descriptor(dtm->get_descriptor(), cto.annotation_type.member_seq[i], dt_map);
      dtm->get_descriptor()->index = i;
      dtm->get_descriptor()->id = i;
      dtm->get_parent() = dt;
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_STRUCTURE:
    dt->get_descriptor()->kind = TK_STRUCTURE;
    dt->get_descriptor()->name = cto.struct_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(0);
    type_identifier_to_dynamic(dt->get_descriptor()->base_type, cto.struct_type.header.base_type, dt_map);
    if (cto.struct_type.struct_flags & IS_FINAL) {
      dt->get_descriptor()->extensibility_kind = FINAL;
    } else if (cto.struct_type.struct_flags & IS_APPENDABLE) {
      dt->get_descriptor()->extensibility_kind = APPENDABLE;
    } else if (cto.struct_type.struct_flags & IS_MUTABLE) {
      dt->get_descriptor()->extensibility_kind = MUTABLE;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("TypeLookupService::complete_to_dynamic_i -")
                 ACE_TEXT(" Invalid extensibility kind in TK_STRUCTURE\n")));
    }
    dt->get_descriptor()->is_nested = cto.struct_type.struct_flags & IS_NESTED;
    for (ACE_CDR::ULong i = 0; i < cto.struct_type.member_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      complete_struct_member_to_member_descriptor(dtm->get_descriptor(), cto.struct_type.member_seq[i], dt_map);
      dtm->get_descriptor()->index = i;
      dtm->get_parent() = dt;
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_UNION:
    dt->get_descriptor()->kind = TK_UNION;
    dt->get_descriptor()->name = cto.union_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(0);
    type_identifier_to_dynamic(dt->get_descriptor()->discriminator_type, cto.union_type.discriminator.common.type_id, dt_map);
    if (cto.union_type.union_flags & IS_FINAL) {
      dt->get_descriptor()->extensibility_kind = FINAL;
    } else if (cto.union_type.union_flags & IS_APPENDABLE) {
      dt->get_descriptor()->extensibility_kind = APPENDABLE;
    } else if (cto.union_type.union_flags & IS_MUTABLE) {
      dt->get_descriptor()->extensibility_kind = MUTABLE;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("TypeLookupService::complete_to_dynamic_i -")
                 ACE_TEXT(" Invalid extensibility kind in TK_UNION\n")));
    }
    dt->get_descriptor()->is_nested = cto.union_type.union_flags & IS_NESTED;
    for (ACE_CDR::ULong i = 0; i < cto.union_type.member_seq.length(); ++i) {
      DynamicTypeMember_rch dtm = DCPS::make_rch<DynamicTypeMember>();
      complete_union_member_to_member_descriptor(dtm->get_descriptor(), cto.union_type.member_seq[i], dt_map);
      dtm->get_descriptor()->index = i;
      dtm->get_parent() = dt;
      dt->insert_dynamic_member(dtm);
    }
    break;
  case TK_BITSET:
    dt->get_descriptor()->kind = TK_BITSET;
    dt->get_descriptor()->name = cto.bitset_type.header.detail.type_name;
    dt->get_descriptor()->bound.length(0);
    break;
  // Collection TKs
  case TK_SEQUENCE:
    dt->get_descriptor()->kind = TK_SEQUENCE;
    if(cto.sequence_type.header.detail.present == true) {
      dt->get_descriptor()->name = cto.sequence_type.header.detail.value.type_name;
    } else {
      dt->get_descriptor()->name = "";
    }
    dt->get_descriptor()->bound.length(1);
    dt->get_descriptor()->bound[0] = cto.sequence_type.header.common.bound;
    type_identifier_to_dynamic(dt->get_descriptor()->element_type, cto.sequence_type.element.common.type, dt_map);
    break;
  case TK_ARRAY: {
    dt->get_descriptor()->kind = TK_ARRAY;
    dt->get_descriptor()->name = cto.array_type.header.detail.type_name;
    dt->get_descriptor()->bound = cto.array_type.header.common.bound_seq;
    type_identifier_to_dynamic(dt->get_descriptor()->element_type, cto.array_type.element.common.type, dt_map);
  }
  break;
  case TK_MAP:
    dt->get_descriptor()->kind = TK_MAP;
    if (cto.map_type.header.detail.present == true) {
      dt->get_descriptor()->name = cto.map_type.header.detail.value.type_name;
    } else {
      dt->get_descriptor()->name = "";
    }
    dt->get_descriptor()->bound.length(1);
    dt->get_descriptor()->bound[0] = cto.map_type.header.common.bound;
    type_identifier_to_dynamic(dt->get_descriptor()->element_type, cto.map_type.element.common.type, dt_map);
    type_identifier_to_dynamic(dt->get_descriptor()->key_element_type, cto.map_type.key.common.type, dt_map);
    break;
  }
}

void TypeLookupService::type_identifier_to_dynamic(DynamicType_rch& dt,
  const TypeIdentifier& ti, DynamicTypeMap& dt_map)
{
  if (ti.kind() == TK_NONE) {
    // Leave as dt as nil
    return;
  }
  DynamicTypeMap::iterator ti_found = dt_map.find(ti);
  if (ti_found != dt_map.end()) {
    dt = ti_found->second;
    return;
  }
  DynamicType_rch dt_instantiation = DCPS::make_rch<DynamicType>();
  dt = dt_instantiation;
  dt_map.insert(dt_map.end(), std::make_pair(ti , dt));
  switch (ti.kind()) {
    case TK_NONE:
      return;
    case TK_BOOLEAN:
      dt->get_descriptor()->kind = TK_BOOLEAN;
      dt->get_descriptor()->name = "Boolean";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_BYTE:
      dt->get_descriptor()->kind = TK_BYTE;
      dt->get_descriptor()->name = "Byte";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_INT16:
      dt->get_descriptor()->kind = TK_INT16;
      dt->get_descriptor()->name = "Int16";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_INT32:
      dt->get_descriptor()->kind = TK_INT32;
      dt->get_descriptor()->name = "Int32";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_INT64:
      dt->get_descriptor()->kind = TK_INT64;
      dt->get_descriptor()->name = "Int64";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_UINT16:
      dt->get_descriptor()->kind = TK_UINT16;
      dt->get_descriptor()->name = "UInt16";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_UINT32:
      dt->get_descriptor()->kind = TK_UINT32;
      dt->get_descriptor()->name = "UInt32";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_UINT64:
      dt->get_descriptor()->kind = TK_UINT64;
      dt->get_descriptor()->name = "UInt64";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_FLOAT32:
      dt->get_descriptor()->kind = TK_FLOAT32;
      dt->get_descriptor()->name = "Float32";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_FLOAT64:
      dt->get_descriptor()->kind = TK_FLOAT64;
      dt->get_descriptor()->name = "Float64";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_FLOAT128:
      dt->get_descriptor()->kind = TK_FLOAT128;
      dt->get_descriptor()->name = "Float128";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_INT8:
      dt->get_descriptor()->kind = TK_INT8;
      dt->get_descriptor()->name = "Int8";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_UINT8:
      dt->get_descriptor()->kind = TK_UINT8;
      dt->get_descriptor()->name = "UInt8";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_CHAR8:
      dt->get_descriptor()->kind = TK_CHAR8;
      dt->get_descriptor()->name = "Char8";
      dt->get_descriptor()->bound.length(0);
      break;
    case TK_CHAR16:
      dt->get_descriptor()->kind = TK_CHAR16;
      dt->get_descriptor()->name = "Char16";
      dt->get_descriptor()->bound.length(0);
      break;
    case TI_STRING8_SMALL:
      dt->get_descriptor()->kind = TK_STRING8;
      dt->get_descriptor()->name = "String8Small";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.string_sdefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, TypeIdentifier(TK_CHAR8), dt_map);
      break;
    case TI_STRING8_LARGE:
      dt->get_descriptor()->kind = TK_STRING8;
      dt->get_descriptor()->name = "String8Large";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.string_sdefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, TypeIdentifier(TK_CHAR8), dt_map);
      break;
    case TI_STRING16_SMALL:
      dt->get_descriptor()->kind = TK_STRING16;
      dt->get_descriptor()->name = "WString16Small";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.string_ldefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, TypeIdentifier(TK_CHAR16), dt_map);
      break;
    case TI_STRING16_LARGE:
      dt->get_descriptor()->kind = TK_STRING16;
      dt->get_descriptor()->name = "WString16Large";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.string_ldefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, TypeIdentifier(TK_CHAR16), dt_map);
      break;
    case TI_PLAIN_SEQUENCE_SMALL:
      dt->get_descriptor()->kind = TK_SEQUENCE;
      dt->get_descriptor()->name = "SequenceSmall";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.seq_sdefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.seq_sdefn().element_identifier, dt_map);
      break;
    case TI_PLAIN_SEQUENCE_LARGE:
      dt->get_descriptor()->kind = TK_SEQUENCE;
      dt->get_descriptor()->name = "SequenceLarge";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.seq_ldefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.seq_ldefn().element_identifier, dt_map);
      break;
    case TI_PLAIN_ARRAY_SMALL:
      dt->get_descriptor()->kind = TK_ARRAY;
      dt->get_descriptor()->name = "ArraySmall";
      dt->get_descriptor()->bound.length(ti.array_sdefn().array_bound_seq.length());
      for (ACE_CDR::ULong i = 0; i< dt->get_descriptor()->bound.length(); ++i) {
        dt->get_descriptor()->bound[i] = ti.array_sdefn().array_bound_seq[i];
      }
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.array_sdefn().element_identifier, dt_map);
      break;
    case TI_PLAIN_ARRAY_LARGE:
      dt->get_descriptor()->kind = TK_ARRAY;
      dt->get_descriptor()->name = "ArrayLarge";
      dt->get_descriptor()->bound = ti.array_ldefn().array_bound_seq;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.array_ldefn().element_identifier, dt_map);
      break;
    case TI_PLAIN_MAP_SMALL:
      dt->get_descriptor()->kind = TK_MAP;
      dt->get_descriptor()->name = "MapSmall";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.map_sdefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.map_sdefn().element_identifier, dt_map);
      type_identifier_to_dynamic(dt->get_descriptor()->key_element_type, *ti.map_sdefn().element_identifier, dt_map);
      break;
    case TI_PLAIN_MAP_LARGE:
      dt->get_descriptor()->kind = TK_MAP;
      dt->get_descriptor()->name = "MapLarge";
      dt->get_descriptor()->bound.length(1);
      dt->get_descriptor()->bound[0] = ti.map_ldefn().bound;
      type_identifier_to_dynamic(dt->get_descriptor()->element_type, *ti.map_ldefn().element_identifier, dt_map);
      type_identifier_to_dynamic(dt->get_descriptor()->key_element_type, *ti.map_ldefn().element_identifier, dt_map);
      break;
    case TI_STRONGLY_CONNECTED_COMPONENT:
        complete_to_dynamic_i(dt, get_type_objects_i(ti).complete, dt_map);
      break;
    case EK_COMPLETE:
        complete_to_dynamic_i(dt, get_type_objects_i(ti).complete, dt_map);
        break;
    case EK_MINIMAL:
      return;
    case TK_ANNOTATION:
      dt->get_descriptor()->kind = TK_ANNOTATION;
      dt->get_descriptor()->name = "Annotation";
      dt->get_descriptor()->bound.length(0);
      break;
  }
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
