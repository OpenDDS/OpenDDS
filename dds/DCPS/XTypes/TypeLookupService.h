/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H
#define OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/SequenceNumber.h>
#include <dds/DCPS/TypeSupportImpl.h>

#include <ace/Thread_Mutex.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export TypeLookupService : public virtual DCPS::RcObject {
public:
  TypeLookupService();
  ~TypeLookupService();

  // For TypeAssignability
  const TypeObject& get_type_objects(const TypeIdentifier& type_id) const;
  void add_type_objects_to_cache(const TypeIdentifier& ti, const TypeObject& tobj);

  // For TypeLookup_getTypes
  void get_type_objects(const TypeIdentifierSeq& type_ids,
    TypeIdentifierTypeObjectPairSeq& types) const;
  void add_type_objects_to_cache(const TypeIdentifierTypeObjectPairSeq& types);

  // For TypeLookup_getTypeDependencies
  bool get_type_dependencies(const TypeIdentifier& type_id,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void get_type_dependencies(const TypeIdentifierSeq& type_ids,
    TypeIdentifierWithSizeSeq& dependencies) const;
  void add_type_dependencies(const TypeIdentifier& type_id,
    const TypeIdentifierWithSizeSeq& dependencies);

  // For adding local endpoint types
  void add_type_objects_to_cache(const DCPS::TypeSupportImpl& typesupport);

  bool type_object_in_cache(const TypeIdentifier& ti) const;
  bool extensibility(TypeFlag extensibility_mask, const TypeIdentifier& ti) const;

private:
  // Only minimal Type Objects for now
  TypeMap minimal_type_map_;

  // For dependencies of local types
  typedef OPENDDS_MAP(TypeIdentifier, TypeIdentifierWithSizeSeq) TypeIdentifierWithSizeSeqMap;
  TypeIdentifierWithSizeSeqMap type_dependencies_map_;

  mutable ACE_Thread_Mutex mutex_;

  TypeObject to_empty_;
};

typedef DCPS::RcHandle<TypeLookupService> TypeLookupService_rch;

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


#endif /* ifndef OPENDDS_DCPS_XTYPES_TYPE_LOOKUP_SERVICE_H */
