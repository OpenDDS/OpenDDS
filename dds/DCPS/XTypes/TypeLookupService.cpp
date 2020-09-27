/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "TypeLookupService.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

TypeLookupService::TypeLookupService() : rpc_sequence_number_(0)
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


void TypeLookupService::add_type_objects_to_cache(TypeIdentifierTypeObjectPairSeq& types)
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


bool TypeLookupService::type_object_in_cache(const TypeIdentifier& ti) const
{
  TypeMap::const_iterator it_type_id_with_size_seq = minimal_type_map_.find(ti);
  return it_type_id_with_size_seq != minimal_type_map_.end();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
