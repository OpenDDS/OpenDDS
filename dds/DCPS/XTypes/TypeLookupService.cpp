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
  for (CORBA::ULong i = 0; i < type_ids.length(); ++i) {
    TypeObjectMap::const_iterator it_object = type_object_map_.find(type_ids[i]);
    if (it_object != type_object_map_.end()) {
      types.append(TypeIdentifierTypeObjectPair(it_object->first, it_object->second));
    }
  }
}

const TypeObject& TypeLookupService::get_type_objects(const TypeIdentifier& type_id) const
{
  const TypeObjectMap::const_iterator it_object = type_object_map_.find(type_id);
  if (it_object != type_object_map_.end()) {
    return it_object->second;
  }
  return to_empty_;
}

bool TypeLookupService::get_type_dependencies(const TypeIdentifier& type_id,
  TypeIdentifierWithSizeSeq& dependencies) const
{
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
  std::set<TypeIdentifier> tmp;
  for (size_t i = 0; i < type_ids.length(); ++i) {
    const TypeIdentifierWithSizeSeqMap::const_iterator it = type_dependencies_map_.find(type_ids[i]);
    if (it != type_dependencies_map_.end()) {
      for (size_t j = 0; j < it->second.length(); ++j) {
        tmp.insert(it->second[j].type_id);
      }
    }
  }

  dependencies.length(tmp.size());
  std::set<TypeIdentifier>::const_iterator iter = tmp.begin();
  size_t i = 0;
  for (; iter != tmp.end(); ++i, ++iter) {
    // TODO(sonndinh): do we include TypeIdentifiers that don't have TypeObjects?
    // Currently, only the ones that have TypeObjects are included.
    const TypeObjectMap::const_iterator tobj_it = type_object_map_.find(*iter);
    if (tobj_it != type_object_map_.end()) {
      dependencies[i].type_id = *iter;
      dependencies[i].typeobject_serialized_size = DCPS::serialized_size(get_typeobject_encoding(), tobj_it->second);
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifierTypeObjectPairSeq& types)
{
  for (ACE_UINT32 i = 0; i < types.length(); ++i) {
    const TypeObjectMap::iterator it_type_id_with_size_seq = type_object_map_.find(types[i].type_identifier);
    if (it_type_id_with_size_seq == type_object_map_.end()) {
      type_object_map_.insert(std::make_pair(types[i].type_identifier, types[i].type_object));
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport)
{
  XTypes::TypeInformation type_info;
  typesupport.to_type_info(type_info);

  const TypeObjectMap::iterator it_type_id_with_size_seq = type_object_map_.find(type_info.minimal.typeid_with_size.type_id);
  if (it_type_id_with_size_seq == type_object_map_.end()) {
    type_object_map_.insert(std::make_pair(type_info.minimal.typeid_with_size.type_id, typesupport.getMinimalTypeObject()));
  }
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  if (type_object_map_.find(ti) == type_object_map_.end()) {
    type_object_map_.insert(std::make_pair(ti, tobj));
  }
}

void TypeLookupService::add_depend_type_identifiers(const TypeIdentifier& type_id,
  const TypeIdentifierWithSizeSeq& dependencies)
{
  if (type_dependencies_map_.find(type_id) == type_dependencies_map_.end()) {
    type_dependencies_map_.insert(std::make_pair(type_id, dependencies));
  }
}

bool TypeLookupService::type_object_in_cache(const TypeIdentifier& ti) const
{
  return type_object_map_.find(ti) != type_object_map_.end();
}

bool TypeLookupService::type_dependencies_in_cache(const TypeIdentifier& ti) const
{
  return type_dependencies_map_.find(ti) != type_dependencies_map_.end();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
