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
    TypeMap::const_iterator it_object = minimal_type_map_.find(type_ids[i]);
    if (it_object != minimal_type_map_.end()) {
      types.append(TypeIdentifierTypeObjectPair(it_object->first, it_object->second));
    }
  }
}

const TypeObject& TypeLookupService::get_type_objects(const TypeIdentifier& type_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, to_empty_);
  const TypeMap::const_iterator it_object = minimal_type_map_.find(type_id);
  if (it_object != minimal_type_map_.end()) {
    return it_object->second;
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
    const TypeMap::const_iterator tobj_it = minimal_type_map_.find(*iter);
    if (tobj_it != minimal_type_map_.end()) {
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
    const TypeMap::iterator it_type_id_with_size_seq = minimal_type_map_.find(types[i].type_identifier);
    if (it_type_id_with_size_seq == minimal_type_map_.end()) {
      minimal_type_map_.insert(std::make_pair(types[i].type_identifier, types[i].type_object));
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  // TODO: This populates the map N times instead of 1.
  const XTypes::TypeMap& minimal_type_map = typesupport.getMinimalTypeMap();
  minimal_type_map_.insert(minimal_type_map.begin(), minimal_type_map.end());
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  TypeMap::const_iterator it_type_id_with_size_seq = minimal_type_map_.find(ti);
  if (it_type_id_with_size_seq == minimal_type_map_.end()) {
    minimal_type_map_.insert(std::make_pair(ti, tobj));
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
  TypeMap::const_iterator it_type_id_with_size_seq = minimal_type_map_.find(ti);
  return it_type_id_with_size_seq != minimal_type_map_.end();
}

bool TypeLookupService::extensibility(TypeFlag extensibility_mask, const TypeIdentifier& type_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  bool result = false;
  TypeObject to = get_type_objects(type_id);

  if (TK_UNION == to.minimal.kind) {
    result = to.minimal.union_type.union_flags & extensibility_mask;
  } else if (TK_STRUCTURE == to.minimal.kind) {
    result = to.minimal.struct_type.struct_flags & extensibility_mask;
  }

  if (result) {
    return true;
  }

  TypeIdentifierWithSizeSeq dependencies;
  TypeIdentifierSeq type_ids;
  type_ids.append(type_id);
  get_type_dependencies(type_ids, dependencies);

  for (unsigned i = 0; i < dependencies.length(); ++i) {
    TypeObject dep_to = get_type_objects(dependencies[i].type_id);
    if (TK_UNION == dep_to.minimal.kind) {
      result = dep_to.minimal.union_type.union_flags & extensibility_mask;
    } else if (TK_STRUCTURE == dep_to.minimal.kind) {
      result = dep_to.minimal.struct_type.struct_flags & extensibility_mask;
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
