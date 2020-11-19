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
    TypeMap::const_iterator it_object = minimal_type_map_.find(type_ids[i]);
    if (it_object != minimal_type_map_.end()) {
      types.append(TypeIdentifierTypeObjectPair(it_object->first, it_object->second));
    }
  }
}

const TypeObject& TypeLookupService::get_type_objects(const TypeIdentifier& type_id) const
{
  const TypeMap::const_iterator it_object = minimal_type_map_.find(type_id);
  if (it_object != minimal_type_map_.end()) {
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
  std::set<TypeIdentifier>::const_iterator iter = tmp.begin();
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
  for (ACE_UINT32 i = 0; i < types.length(); ++i) {
    const TypeMap::iterator it_type_id_with_size_seq = minimal_type_map_.find(types[i].type_identifier);
    if (it_type_id_with_size_seq == minimal_type_map_.end()) {
      minimal_type_map_.insert(std::make_pair(types[i].type_identifier, types[i].type_object));
    }
  }
}

void TypeLookupService::add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport)
{
  // TODO: This populates the map N times instead of 1.
  const XTypes::TypeMap& minimal_type_map = typesupport.getMinimalTypeMap();
  minimal_type_map_.insert(minimal_type_map.begin(), minimal_type_map.end());
}

void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  TypeMap::const_iterator it_type_id_with_size_seq = minimal_type_map_.find(ti);
  if (it_type_id_with_size_seq == minimal_type_map_.end()) {
    minimal_type_map_.insert(std::make_pair(ti, tobj));
  }
}

void TypeLookupService::add_type_dependencies(const TypeIdentifier& type_id,
  const TypeIdentifierWithSizeSeq& dependencies)
{
  if (type_dependencies_map_.find(type_id) == type_dependencies_map_.end()) {
    type_dependencies_map_.insert(std::make_pair(type_id, dependencies));
  }
}

bool TypeLookupService::type_object_in_cache(const TypeIdentifier& ti) const
{
  TypeMap::const_iterator it_type_id_with_size_seq = minimal_type_map_.find(ti);
  return it_type_id_with_size_seq != minimal_type_map_.end();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
