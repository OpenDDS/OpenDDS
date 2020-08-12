#ifndef OPENDDS_RTPS_TYPE_LOOKUP_H_
#define OPENDDS_RTPS_TYPE_LOOKUP_H_

/*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/
#include "RtpsRpcTypeSupportImpl.h"
#include <dds/DCPS/XTypes/TypeObject.h>
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// As per chapter 7.6.3.3.3 of XTypes spec
// Used in TypeLookup_Call and TypeLookup_Return
const CORBA::ULong TypeLookup_getTypes_HashId = 25318099U;
const CORBA::ULong TypeLookup_getDependencies_HashId = 95091505U;

struct TypeLookup_getTypes_In
{
  TypeIdentifierSeq type_ids;

  TypeLookup_getTypes_In() {}
};

struct TypeLookup_getTypes_Out
{
  TypeIdentifierTypeObjectPairSeq types;
  TypeIdentifierPairSeq complete_to_minimal;

  TypeLookup_getTypes_Out() {}
};

struct TypeLookup_getTypes_Result
{
  CORBA::ULong return_code;
  TypeLookup_getTypes_Out result;

  TypeLookup_getTypes_Result() {}
};

struct OctetSeq32 : Sequence<ACE_CDR::Octet>
{
};

struct TypeLookup_getTypeDependencies_In
{
  TypeIdentifierSeq type_ids;
  OctetSeq32 continuation_point;

  TypeLookup_getTypeDependencies_In() {}
};

struct TypeLookup_getTypeDependencies_Out
{
  TypeIdentifierWithSizeSeq dependent_typeids;
  OctetSeq32 continuation_point;

  TypeLookup_getTypeDependencies_Out() {}
};

struct TypeLookup_getTypeDependencies_Result
{
  CORBA::ULong return_code;
  TypeLookup_getTypeDependencies_Out result;

  TypeLookup_getTypeDependencies_Result() {}
};

struct TypeLookup_Call
{
  CORBA::ULong kind;
  TypeLookup_getTypes_In getTypes;
  TypeLookup_getTypeDependencies_In getTypeDependencies;

  TypeLookup_Call() {}
};

struct TypeLookup_Request
{
  DDS::RPC::RequestHeader header;
  TypeLookup_Call data;

  TypeLookup_Request() {}
};

struct TypeLookup_Return
{
  CORBA::ULong kind;
  TypeLookup_getTypes_Result getTypes;
  TypeLookup_getTypeDependencies_Result getTypeDependencies;

  TypeLookup_Return() {}
};

struct TypeLookup_Reply
{
  DDS::RPC::ResponseHeader header;
  TypeLookup_Return data;

  TypeLookup_Reply() {}
};
} // namespace XTypes

namespace DCPS {

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypes_In& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypes_In& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypes_In& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypes_Out& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypes_Out& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypes_Out& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypes_Result& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypes_Result& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypes_Result& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_In& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_Out& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Out& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Out& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_Result& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Result& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Result& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Call& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_Call& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_Call& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Request& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_Request& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_Request& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Return& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_Return& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_Return& stru);

void serialized_size(const DCPS::Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Reply& stru);
bool operator<<(DCPS::Serializer& strm, const XTypes::TypeLookup_Reply& stru);
bool operator>>(DCPS::Serializer& strm, XTypes::TypeLookup_Reply& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::OctetSeq32& seq);
bool operator<<(Serializer& strm, const XTypes::OctetSeq32& seq);
bool operator>>(Serializer& strm, XTypes::OctetSeq32& seq);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* ifndef OPENDDS_RTPS_TYPE_LOOKUP_H_ */
