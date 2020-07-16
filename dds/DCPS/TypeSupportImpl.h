/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPESUPPORTIMPL_H
#define OPENDDS_DCPS_TYPESUPPORTIMPL_H

#include "dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DdsDcpsTypeSupportExtC.h"
#include "LocalObject.h"
#include "Serializer.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL


namespace OpenDDS {

namespace XTypes {
  struct TypeInformation;
  struct TypeObject;
}

namespace DCPS {

class MetaStruct;

template <typename Message> struct DDSTraits;

template <typename Message> struct MarshalTraits;

class OpenDDS_Dcps_Export TypeSupportImpl
  : public virtual LocalObject<TypeSupport> {
public:
  TypeSupportImpl() {}

  virtual ~TypeSupportImpl();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual const MetaStruct& getMetaStructForType() = 0;
#endif

  virtual DDS::ReturnCode_t register_type(DDS::DomainParticipant_ptr participant,
                                          const char* type_name);

  virtual DDS::ReturnCode_t unregister_type(DDS::DomainParticipant_ptr participant,
                                            const char* type_name);

  virtual char* get_type_name();

  /// Get allowed representations defined by IDL annotations
  virtual void representations_allowed_by_type(
    DDS::DataRepresentationIdSeq& seq) = 0;

  virtual const XTypes::TypeObject& getMinimalTypeObject() const = 0;

  virtual Extensibility getExtensibility() const = 0;

  void to_type_info(XTypes::TypeInformation& type_info) const;

private:
  virtual const char* default_type_name() const = 0;

  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(TypeSupportImpl)
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
