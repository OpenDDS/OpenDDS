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
  type_map_.insert(minimal_type_map.begin(), minimal_type_map.end());
  type_map_.insert(complete_type_map.begin(), complete_type_map.end());
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
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

DDS::ReturnCode_t TypeLookupService::complete_struct_member_to_member_descriptor(MemberDescriptor*& md,
  const CompleteStructMember& cm) {
  md->name = cm.detail.name;
  md->id = cm.common.member_id;
  TypeObject to = get_type_objects_i(cm.common.member_type_id);
  complete_to_dynamic(md->type, to.complete);
  md->default_value = ""; //TODO CLAYTON: Where do we get the default value from CompleteStructMember?
  md->label.length(0);
  if (cm.common.member_flags & (1 << 0)) {
    if (cm.common.member_flags & (1 << 1)) {
      md->try_construct_kind = TRIM ;
    } else {
      md->try_construct_kind = DISCARD;
    }
  } else {
    if (cm.common.member_flags & (1 << 1)) {
      md->try_construct_kind = USE_DEFAULT;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Invalid TryConstruct Kind in complete_struct_member_to_member_descriptor\n")));
    }
  }
  md->is_key = (cm.common.member_flags & (1 << 5));
  md->is_optional = (cm.common.member_flags & (1 << 3));
  md->is_must_understand = (cm.common.member_flags & (1 << 4));
  md->is_shared = (cm.common.member_flags & (1 << 2));
  md->is_default_label = 0;
}

DDS::ReturnCode_t TypeLookupService::complete_struct_member_to_dynamic_type_member(DynamicTypeMember_rch& dtm,
  const CompleteStructMember& cm) {
  complete_struct_member_to_member_descriptor(dtm->descriptor_, cm);
}

DDS::ReturnCode_t TypeLookupService::complete_to_dynamic(DynamicType_rch& dt, const CompleteTypeObject& cto) {
  switch (cto.kind) {
  //primitive TKs
  case TK_NONE:
    dt->descriptor_->kind = TK_NONE;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_BOOLEAN:
    dt->descriptor_->kind = TK_BOOLEAN;
    dt->descriptor_->name = "bool";
    dt->descriptor_->bound.length(0);
    break;
  case TK_BYTE:
    dt->descriptor_->kind = TK_BYTE;
    dt->descriptor_->name = "byte";
    dt->descriptor_->bound.length(0);
    break;
  case TK_INT16:
    dt->descriptor_->kind = TK_INT16;
    dt->descriptor_->name = "short";
    dt->descriptor_->bound.length(0);
    break;
  case TK_INT32:
    dt->descriptor_->kind = TK_INT32;
    dt->descriptor_->name = "long";
    dt->descriptor_->bound.length(0);
    break;
  case TK_INT64:
    dt->descriptor_->kind = TK_INT64;
    dt->descriptor_->name = "longlong";
    dt->descriptor_->bound.length(0);
    break;
  case TK_UINT16:
    dt->descriptor_->kind = TK_UINT16;
    dt->descriptor_->name = "ushort";
    dt->descriptor_->bound.length(0);
    break;
  case TK_UINT32:
    dt->descriptor_->kind = TK_UINT32;
    dt->descriptor_->name = "ulong";
    dt->descriptor_->bound.length(0);
    break;
  case TK_UINT64:
    dt->descriptor_->kind = TK_UINT64;
    dt->descriptor_->name = "ulonglong";
    dt->descriptor_->bound.length(0);
    break;
  case TK_FLOAT32:
    dt->descriptor_->kind = TK_FLOAT32;
    dt->descriptor_->name = "double";
    dt->descriptor_->bound.length(0);
    break;
  case TK_FLOAT64:
    dt->descriptor_->kind = TK_FLOAT32;
    dt->descriptor_->name = "longdouble";
    dt->descriptor_->bound.length(0);
    break;
  case TK_FLOAT128:
    dt->descriptor_->kind = TK_FLOAT32;
    dt->descriptor_->name = "longlongdouble";
    dt->descriptor_->bound.length(0);
    break;
  case TK_INT8:
    dt->descriptor_->kind = TK_INT8;
    dt->descriptor_->name = ""; // TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_UINT8:
    dt->descriptor_->kind = TK_UINT8;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_CHAR8:
    dt->descriptor_->kind = TK_CHAR8;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_CHAR16:
    dt->descriptor_->kind = TK_CHAR16;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  //string TKs
  case TK_STRING8:
    dt->descriptor_->kind = TK_STRING8;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_STRING16:
    dt->descriptor_->kind = TK_STRING16;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  // Constructed/Named types
  case TK_ALIAS:
    break;
  // Enumerated TKs
  case TK_ENUM:
    dt->descriptor_->kind = TK_ENUM;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_BITMASK:
    dt->descriptor_->kind = TK_BITMASK;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  // Structured TKs
  case TK_ANNOTATION:
    dt->descriptor_->kind = TK_ANNOTATION;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_STRUCTURE: {
    dt->descriptor_->kind = TK_STRUCTURE;
    dt->descriptor_->name = cto.struct_type.header.detail.type_name;
    //dt->descriptor_->discriminator_type = nil
    dt->descriptor_->bound.length(0);
    
    TypeObject to = get_type_objects_i(cto.struct_type.header.base_type);
    complete_to_dynamic(dt->descriptor_->base_type, to.complete);
    
    //dt->descriptor_->element_type =  nil
    //dt->descriptor_->key_element_type =  nil
    if (cto.struct_type.struct_flags & (1 << 0)) {
      dt->descriptor_->extensibility_kind = FINAL;
    } else if (cto.struct_type.struct_flags & (1 << 1)) {
      dt->descriptor_->extensibility_kind = APPENDABLE;
    } else if (cto.struct_type.struct_flags & (1 << 2)) {
      dt->descriptor_->extensibility_kind = MUTABLE;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Invalid extensibility kind in complete_to_dynamic(DynamicType& dt, const CompleteTypeObject& cto)\n")));
    }
    dt->descriptor_->is_nested = (cto.struct_type.struct_flags & (1 << 3));
    for (ulong i = 0; i < cto.struct_type.member_seq.length(); ++i) {
      DynamicTypeMember_rch dtm(new DynamicTypeMember, OpenDDS::DCPS::keep_count());
      complete_struct_member_to_dynamic_type_member(dtm, cto.struct_type.member_seq[i]);
      dt->member_by_index.insert(dt->member_by_index.end(), dtm); //insert at end?
    }
  }
  break;
  case TK_UNION:
    dt->descriptor_->kind = TK_UNION;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_BITSET:
    dt->descriptor_->kind = TK_BITSET;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  // Collection TKs
  case TK_SEQUENCE:
    dt->descriptor_->kind = TK_SEQUENCE;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_ARRAY:
    dt->descriptor_->kind = TK_ARRAY;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
    break;
  case TK_MAP:
    dt->descriptor_->kind = TK_MAP;
    dt->descriptor_->name = ""; //TODO CLAYTON
    dt->descriptor_->bound.length(0);
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
