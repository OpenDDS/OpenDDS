/*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef OPENDDS_DCPS_FLEXIBLETYPESUPPORT_H
#define OPENDDS_DCPS_FLEXIBLETYPESUPPORT_H

#include "dcps_export.h"

#include "TypeSupportImpl.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export FlexibleTypeSupport : public TypeSupportImpl {
public:
  FlexibleTypeSupport(const RcHandle<TypeSupportImpl>& baseTypeSupport,
                      const String& name)
    : base_(baseTypeSupport)
    , name_(name)
  {}

  const char* name() const { return name_.c_str(); }
  char* get_type_name();

  DDS::ReturnCode_t add(const RcHandle<TypeSupportImpl>& alternativeTypeSupport);

  void to_type_info(TypeInformation& type_info) const;
  const XTypes::TypeIdentifier& getMinimalTypeIdentifier() const;
  const XTypes::TypeMap& getMinimalTypeMap() const;
  const XTypes::TypeIdentifier& getCompleteTypeIdentifier() const;
  const XTypes::TypeMap& getCompleteTypeMap() const;

  // the following functions simply delegate to base_
  const MetaStruct& getMetaStructForType() const { return base_->getMetaStructForType(); }
  size_t key_count() const { return base_->key_count(); }
  bool is_dcps_key(const char* fieldname) const { return base_->is_dcps_key(fieldname); }
  SerializedSizeBound serialized_size_bound(const Encoding& encoding) const { return base_->serialized_size_bound(encoding); }
  SerializedSizeBound key_only_serialized_size_bound(const Encoding& encoding) const { return base_->key_only_serialized_size_bound(encoding); }
  Extensibility base_extensibility() const { return base_->base_extensibility(); }
  Extensibility max_extensibility() const { return base_->max_extensibility(); }

private:
  RcHandle<TypeSupportImpl> base_;
  String name_;
  OPENDDS_MAP(String, RcHandle<TypeSupportImpl>) map_;

  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(FlexibleTypeSupport)
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
