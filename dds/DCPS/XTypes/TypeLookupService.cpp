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
  rpc_sequence_number_ = 0;
}


TypeLookupService::~TypeLookupService()
{
}


void TypeLookupService::collect_types_info_from_cache(const TypeIdentifierSeq& type_ids,
                                                      TypeIdentifierTypeObjectPairSeq& types,
                                                      TypeIdentifierPairSeq& complete_to_minimal,
                                                      TypeIdentifierSeq& not_found)
{
  for (CORBA::ULong i = 0; i < type_ids.length(); ++i) {
    TypeObjectMap::iterator it_object = type_object_map_.find(type_ids[i]);
    if (it_object != type_object_map_.end()) {
      TypeIdentifierTypeObjectPair new_to_pair(it_object->first, it_object->second);
      types.append(new_to_pair);
    } else {
      not_found.append(type_ids[i]);
    }
  }
}


void TypeLookupService::get_type_objects(const TypeIdentifierSeq& type_ids,
                                         TypeIdentifierTypeObjectPairSeq& types,
                                         TypeIdentifierPairSeq& complete_to_minimal)
{
  TypeIdentifierSeq dummy;
  collect_types_info_from_cache(type_ids, types, complete_to_minimal, dummy);
}


const TypeObject& TypeLookupService::get_type_objects(const TypeIdentifier& type_id) const
{
  TypeObjectMap::const_iterator it_object = type_object_map_.find(type_id);
  if (it_object != type_object_map_.end()) {
    return it_object->second;
  }
  return to_empty_;
}


void TypeLookupService::add_type_objects_to_cache(TypeIdentifierTypeObjectPairSeq& types)
{
  for (ACE_UINT32 i = 0; i < types.length(); ++i) {
    TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map_.find(types[i].type_identifier);
    if (it_type_id_with_size_seq == type_dependencies_map_.end()) {
      type_object_map_.insert(std::pair<TypeIdentifier, TypeObject>(types[i].type_identifier, types[i].type_object));
    }
  }
}


void TypeLookupService::add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport)
{
  XTypes::TypeInformation type_info;
  typesupport.to_type_info(type_info);

  TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map_.find(type_info.minimal.typeid_with_size.type_id);
  if (it_type_id_with_size_seq == type_dependencies_map_.end()) {
    type_object_map_.insert(std::pair<TypeIdentifier, TypeObject>(type_info.minimal.typeid_with_size.type_id,
                                                                  typesupport.getMinimalTypeObject()));
  }
}


void TypeLookupService::add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map_.find(ti);
  if (it_type_id_with_size_seq == type_dependencies_map_.end()) {
    type_object_map_.insert(std::pair<TypeIdentifier, TypeObject>(ti, tobj));
  }
}


bool TypeLookupService::type_object_in_cache(const TypeIdentifier& ti)
{
  TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map_.find(ti);
  return it_type_id_with_size_seq != type_dependencies_map_.end();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
