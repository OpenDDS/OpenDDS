#pragma once
#ifndef OPENDDS_DCPS_TYPE_LOOKUP_SERVICE_H
#define OPENDDS_DCPS_TYPE_LOOKUP_SERVICE_H

/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/SequenceNumber.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export TypeLookupService : public virtual DCPS::RcObject {
public:
  TypeLookupService();
  ~TypeLookupService();

  // For TypeAssignability
  const TypeObject& GetTypeObject(const TypeIdentifier& type_id) const;
  void AddTypeObjectsToCache(const TypeIdentifier& ti, const TypeObject& tobj);

  // For Type Lookup reply
  void GetTypeObjects(const TypeIdentifierSeq& type_ids,
    TypeIdentifierTypeObjectPairSeq& types,
    TypeIdentifierPairSeq& complete_to_minimal);
  void AddTypeObjectsToCache(TypeIdentifierTypeObjectPairSeq& types);

  bool TypeObjectInCache(const TypeIdentifier& ti);

  OpenDDS::DCPS::SequenceNumber rpc_sequence_number_;

protected:
  // Only minimal Type Objects for now
  typedef std::map<TypeIdentifier, TypeObject> TypeObjectMap;
  TypeObjectMap type_object_map;

  typedef std::map<TypeIdentifier, TypeIdentifierWithSizeSeq> TypeIdentifierWithSizeSeqMap;
  TypeIdentifierWithSizeSeqMap type_dependencies_map;

  void CollectTypesInfoFromCache(const TypeIdentifierSeq& type_ids,
    TypeIdentifierTypeObjectPairSeq& types,
    TypeIdentifierPairSeq& complete_to_minimal,
    TypeIdentifierSeq& not_found);

  TypeObject to_empty;
};

typedef DCPS::RcHandle<TypeLookupService> TypeLookupService_rch;
} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


#endif /* ifndef OPENDDS_DCPS_TYPE_LOOKUP_SERVICE_H */
