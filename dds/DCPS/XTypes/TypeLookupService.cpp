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
  to_empty.minimal.kind = TK_NONE;
  to_empty.complete.kind = TK_NONE;
  rpc_sequence_number_ = 0;
}


TypeLookupService::~TypeLookupService()
{
}


void TypeLookupService::CollectTypesInfoFromCache(const TypeIdentifierSeq& type_ids,
                                                  TypeIdentifierTypeObjectPairSeq& types,
                                                  TypeIdentifierPairSeq& complete_to_minimal,
                                                  TypeIdentifierSeq& not_found)
{
  for (CORBA::ULong i = 0; i < type_ids.length(); ++i) {
    TypeObjectMap::iterator it_object = type_object_map.find(type_ids[i]);
    if (it_object != type_object_map.end()) {
      TypeIdentifierTypeObjectPair new_to_pair(it_object->first, it_object->second);
      types.append(new_to_pair);
    }
    else {
      not_found.append(type_ids[i]);
    }
  }
}


void TypeLookupService::GetTypeObjects(const TypeIdentifierSeq& type_ids,
                                       TypeIdentifierTypeObjectPairSeq& types,
                                       TypeIdentifierPairSeq& complete_to_minimal)
{
  TypeIdentifierSeq dummy;
  CollectTypesInfoFromCache(type_ids, types, complete_to_minimal, dummy);
}


const TypeObject& TypeLookupService::GetTypeObject(const TypeIdentifier& type_id) const
{
  TypeObjectMap::const_iterator it_object = type_object_map.find(type_id);
  if (it_object != type_object_map.end()) {
    return it_object->second;
  }
  return to_empty;
}


void TypeLookupService::AddTypeObjectsToCache(TypeIdentifierTypeObjectPairSeq& types)
{
  for (ACE_UINT32 i = 0; i < types.length(); ++i)
  {
    TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map.find(types[i].type_identifier);
    if (it_type_id_with_size_seq == type_dependencies_map.end()) {
      type_object_map.insert(std::pair<TypeIdentifier, TypeObject>(types[i].type_identifier, types[i].type_object));
    }
  }
}


void TypeLookupService::AddTypeObjectsToCache(const TypeIdentifier& ti, const TypeObject& tobj)
{
  TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map.find(ti);
  if (it_type_id_with_size_seq == type_dependencies_map.end()) {
    type_object_map.insert(std::pair<TypeIdentifier, TypeObject>(ti, tobj));
  }
}


bool TypeLookupService::TypeObjectInCache(const TypeIdentifier& ti)
{
  TypeIdentifierWithSizeSeqMap::iterator it_type_id_with_size_seq = type_dependencies_map.find(ti);
  return it_type_id_with_size_seq != type_dependencies_map.end();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
