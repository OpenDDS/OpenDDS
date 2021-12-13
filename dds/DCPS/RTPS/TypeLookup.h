/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_TYPE_LOOKUP_H
#define OPENDDS_DCPS_RTPS_TYPE_LOOKUP_H

#include "BaseMessageTypes.h"
#include "RtpsRpcTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// As per chapter 7.6.3.3.3 of XTypes spec
// Issue DDSXTY14-30:
// - These use XCDR2 and default to appendable.
// - The @RPCRequestType/@RPCReplyType structs are final.

// Used in TypeLookup_Call and TypeLookup_Return
const ACE_CDR::ULong TypeLookup_getTypes_HashId = 0x018252d3;
const ACE_CDR::ULong TypeLookup_getDependencies_HashId = 0x05aafb31;

// @mutable
struct TypeLookup_getTypes_In {
  // @hashid
  TypeIdentifierSeq type_ids;

  TypeLookup_getTypes_In() {}
};

// @mutable
struct TypeLookup_getTypes_Out {
  // @hashid
  TypeIdentifierTypeObjectPairSeq types;
  // @hashid
  TypeIdentifierPairSeq complete_to_minimal;

  TypeLookup_getTypes_Out() {}
};

struct TypeLookup_getTypes_Result { //IDL: union switch (long)
  ACE_CDR::Long return_code;

  // case DDS_RETCODE_OK:
  TypeLookup_getTypes_Out result;

  TypeLookup_getTypes_Result()
    : return_code(DDS::RETCODE_OK)
  {}
};

struct OctetSeq32 : Sequence<ACE_CDR::Octet> {
};

// @mutable
struct TypeLookup_getTypeDependencies_In {
  // @hashid
  TypeIdentifierSeq type_ids;
  // @hashid
  OctetSeq32 continuation_point;

  TypeLookup_getTypeDependencies_In() {}
};

// @mutable
struct TypeLookup_getTypeDependencies_Out {
  // @hashid
  TypeIdentifierWithSizeSeq dependent_typeids;
  // @hashid
  OctetSeq32 continuation_point;

  TypeLookup_getTypeDependencies_Out() {}
};

struct TypeLookup_getTypeDependencies_Result { //IDL: union switch (long)
  ACE_CDR::Long return_code;

  // case DDS_RETCODE_OK:
  TypeLookup_getTypeDependencies_Out result;

  TypeLookup_getTypeDependencies_Result()
    : return_code(DDS::RETCODE_OK)
  {}
};

struct TypeLookup_Call { //IDL: union switch (long)
  ACE_CDR::Long kind;

  // case TypeLookup_getTypes_Hash:
  TypeLookup_getTypes_In getTypes;

  // case TypeLookup_getDependencies_Hash:
  TypeLookup_getTypeDependencies_In getTypeDependencies;

  TypeLookup_Call()
    : kind(static_cast<ACE_CDR::Long>(TypeLookup_getTypes_HashId))
  {}
};

// @final @RPCRequestType
struct TypeLookup_Request {
  DDS::RPC::RequestHeader header;
  TypeLookup_Call data;

  TypeLookup_Request()
  {
    header.requestId.writer_guid = RTPS::GUID_UNKNOWN;
    header.requestId.sequence_number = RTPS::SEQUENCENUMBER_UNKNOWN;
  }
};

struct TypeLookup_Return { //IDL: union switch (long)
  ACE_CDR::Long kind;

  // case TypeLookup_getTypes_Hash:
  TypeLookup_getTypes_Result getType;

  // case TypeLookup_getDependencies_Hash:
  TypeLookup_getTypeDependencies_Result getTypeDependencies;

  TypeLookup_Return()
    : kind(static_cast<ACE_CDR::Long>(TypeLookup_getTypes_HashId))
  {}
};

// @final @RPCRequestType
struct TypeLookup_Reply {
  DDS::RPC::ReplyHeader header;
  TypeLookup_Return _cxx_return;

  TypeLookup_Reply()
  {
    header.relatedRequestId.writer_guid = RTPS::GUID_UNKNOWN;
    header.relatedRequestId.sequence_number = RTPS::SEQUENCENUMBER_UNKNOWN;
    header.remoteEx = DDS::RPC::REMOTE_EX_OK;
  }
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

#endif /* ifndef OPENDDS_DCPS_RTPS_TYPE_LOOKUP_H */
