#include "DCPS/DdsDcps_pch.h"
#include "TypeLookup_getTypeDependencies_InTypeSupportImpl.h"
#include <cstring>
#include <stdexcept>
#include "dds/CorbaSeq/OctetSeqTypeSupportImpl.h"
#include "dds/DCPS/FilterEvaluator.h"
#include "dds/DCPS/PoolAllocator.h"


/* Begin TYPEDEF: TypeIdentifierSeq */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::TypeIdentifierSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  OpenDDS::DCPS::serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    serialized_size(encoding, size, seq[i]);
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm << seq[i])) {
      return false;
    }
  }
  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm >> seq[i])) {
      return false;
    }
  }
  return true;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

bool gen_skip_over(Serializer& ser, XTypes::TypeIdentifierSeq*)
{
  ACE_UNUSED_ARG(ser);
  ACE_CDR::ULong length;
  if (!(ser >> length)) return false;
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!gen_skip_over(ser, static_cast<XTypes::TypeIdentifier*>(0))) return false;
  }
  return true;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_TypeIdentifierSeq_xtag>()
{
  static const XTypes::TypeObject to = XTypes::TypeObject(
    XTypes::MinimalTypeObject(
      XTypes::MinimalAliasType(
        XTypes::AliasTypeFlag(0),
        XTypes::MinimalAliasHeader(),
        XTypes::MinimalAliasBody(
          XTypes::CommonAliasBody(
            XTypes::AliasMemberFlag(),
            XTypes::TypeIdentifier::makePlainSequence(getMinimalTypeIdentifier<XTypes_TypeIdentifier_xtag>(), XTypes::SBound(0))
          )
        )
      )
    )
  );
  return to;
}

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_TypeIdentifierSeq_xtag>()
{
  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<XTypes_TypeIdentifierSeq_xtag>());
  return ti;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL


/* End TYPEDEF: TypeIdentifierSeq */


/* Begin TYPEDEF: continuation_point_Seq */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::continuation_point_Seq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  OpenDDS::DCPS::serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  max_serialized_size_octet(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const XTypes::continuation_point_Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const CORBA::ULong length = seq.length();
  if (length > 32) {
    return false;
  }
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_octet_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, XTypes::continuation_point_Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > seq.maximum()) {
    return false;
  }
  seq.length(length);
  if (length == 0) {
    return true;
  }
  return strm.read_octet_array(seq.get_buffer(), length);
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

bool gen_skip_over(Serializer& ser, XTypes::continuation_point_Seq*)
{
  ACE_UNUSED_ARG(ser);
  ACE_CDR::ULong length;
  if (!(ser >> length)) return false;
  return ser.skip(static_cast<ACE_UINT16>(length), 1);
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_continuation_point_Seq_xtag>()
{
  static const XTypes::TypeObject to = XTypes::TypeObject(
    XTypes::MinimalTypeObject(
      XTypes::MinimalAliasType(
        XTypes::AliasTypeFlag(0),
        XTypes::MinimalAliasHeader(),
        XTypes::MinimalAliasBody(
          XTypes::CommonAliasBody(
            XTypes::AliasMemberFlag(),
            XTypes::TypeIdentifier::makePlainSequence(getMinimalTypeIdentifier<CORBA::Octet>(), XTypes::SBound(32))
          )
        )
      )
    )
  );
  return to;
}

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_continuation_point_Seq_xtag>()
{
  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<XTypes_continuation_point_Seq_xtag>());
  return ti;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL


/* End TYPEDEF: continuation_point_Seq */


/* Begin STRUCT: TypeLookup_getTypeDependencies_In */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(stru);
  serialized_size(encoding, size, stru.type_ids);
  serialized_size(encoding, size, stru.continuation_point);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(stru);
  return (strm << stru.type_ids)
    && (strm << stru.continuation_point);
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(stru);
  return (strm >> stru.type_ids)
    && (strm >> stru.continuation_point);
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

template<>
struct MetaStructImpl<XTypes::TypeLookup_getTypeDependencies_In> : MetaStruct {
  typedef XTypes::TypeLookup_getTypeDependencies_In T;

#ifndef OPENDDS_NO_MULTI_TOPIC
  void* allocate() const { return new T; }

  void deallocate(void* stru) const { delete static_cast<T*>(stru); }

  size_t numDcpsKeys() const { return 0; }

#endif /* OPENDDS_NO_MULTI_TOPIC */

  bool isDcpsKey(const char* field) const
  {
    ACE_UNUSED_ARG(field);
    return false;
  }

  Value getValue(const void* stru, const char* field) const
  {
    const XTypes::TypeLookup_getTypeDependencies_In& typed = *static_cast<const XTypes::TypeLookup_getTypeDependencies_In*>(stru);
    ACE_UNUSED_ARG(typed);
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct XTypes::TypeLookup_getTypeDependencies_In)");
  }

  Value getValue(Serializer& ser, const char* field) const
  {
    if (!gen_skip_over(ser, static_cast<XTypes::TypeIdentifierSeq*>(0))) {
      throw std::runtime_error("Field " + OPENDDS_STRING(field) + " could not be skipped");
    }
    if (!gen_skip_over(ser, static_cast<XTypes::continuation_point_Seq*>(0))) {
      throw std::runtime_error("Field " + OPENDDS_STRING(field) + " could not be skipped");
    }
    if (!field[0]) {
      return 0;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not valid for struct XTypes::TypeLookup_getTypeDependencies_In");
  }

  ComparatorBase::Ptr create_qc_comparator(const char* field, ComparatorBase::Ptr next) const
  {
    ACE_UNUSED_ARG(next);
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct XTypes::TypeLookup_getTypeDependencies_In)");
  }

#ifndef OPENDDS_NO_MULTI_TOPIC
  const char** getFieldNames() const
  {
    static const char* names[] = {"type_ids", "continuation_point", 0};
    return names;
  }

  const void* getRawField(const void* stru, const char* field) const
  {
    if (std::strcmp(field, "type_ids") == 0) {
      return &static_cast<const T*>(stru)->type_ids;
    }
    if (std::strcmp(field, "continuation_point") == 0) {
      return &static_cast<const T*>(stru)->continuation_point;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct XTypes::TypeLookup_getTypeDependencies_In)");
  }

  void assign(void* lhs, const char* field, const void* rhs,
    const char* rhsFieldSpec, const MetaStruct& rhsMeta) const
  {
    ACE_UNUSED_ARG(lhs);
    ACE_UNUSED_ARG(field);
    ACE_UNUSED_ARG(rhs);
    ACE_UNUSED_ARG(rhsFieldSpec);
    ACE_UNUSED_ARG(rhsMeta);
    if (std::strcmp(field, "type_ids") == 0) {
      static_cast<T*>(lhs)->type_ids = *static_cast<const XTypes::TypeIdentifierSeq*>(rhsMeta.getRawField(rhs, rhsFieldSpec));
      return;
    }
    if (std::strcmp(field, "continuation_point") == 0) {
      static_cast<T*>(lhs)->continuation_point = *static_cast<const XTypes::continuation_point_Seq*>(rhsMeta.getRawField(rhs, rhsFieldSpec));
      return;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct XTypes::TypeLookup_getTypeDependencies_In)");
  }
#endif /* OPENDDS_NO_MULTI_TOPIC */

  bool compare(const void* lhs, const void* rhs, const char* field) const
  {
    ACE_UNUSED_ARG(lhs);
    ACE_UNUSED_ARG(field);
    ACE_UNUSED_ARG(rhs);
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct XTypes::TypeLookup_getTypeDependencies_In)");
  }
};

template<>
const MetaStruct& getMetaStruct<XTypes::TypeLookup_getTypeDependencies_In>()
{
  static MetaStructImpl<XTypes::TypeLookup_getTypeDependencies_In> msi;
  return msi;
}

bool gen_skip_over(Serializer& ser, XTypes::TypeLookup_getTypeDependencies_In*)
{
  ACE_UNUSED_ARG(ser);
  MetaStructImpl<XTypes::TypeLookup_getTypeDependencies_In>().getValue(ser, "");
  return true;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_TypeLookup_getTypeDependencies_In_xtag>()
{
  static const XTypes::TypeObject to = XTypes::TypeObject(
    XTypes::MinimalTypeObject(
      XTypes::MinimalStructType(
        XTypes::StructTypeFlag( XTypes::IS_APPENDABLE | XTypes::IS_NESTED ),
        XTypes::MinimalStructHeader(
          getMinimalTypeIdentifier<void>(),
          XTypes::MinimalTypeDetail()
        ),
        XTypes::MinimalStructMemberSeq()
        .append(
          XTypes::MinimalStructMember(
            XTypes::CommonStructMember(
              0,
              XTypes::StructMemberFlag( 0 ),
              getMinimalTypeIdentifier<XTypes_TypeIdentifierSeq_xtag>()
            ),
            XTypes::MinimalMemberDetail("type_ids")
          )
        )
        .append(
          XTypes::MinimalStructMember(
            XTypes::CommonStructMember(
              1,
              XTypes::StructMemberFlag( 0 ),
              getMinimalTypeIdentifier<XTypes_continuation_point_Seq_xtag>()
            ),
            XTypes::MinimalMemberDetail("continuation_point")
          )
        )
        .sort()
        )
      )
    );
  return to;
}

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_TypeLookup_getTypeDependencies_In_xtag>()
{
  static const XTypes::TypeIdentifier ti = XTypes::makeTypeIdentifier(getMinimalTypeObject<XTypes_TypeLookup_getTypeDependencies_In_xtag>());
  return ti;
}

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL
