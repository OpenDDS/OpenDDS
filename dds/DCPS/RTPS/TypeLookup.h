/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_TYPE_LOOKUP_H
#define OPENDDS_DCPS_RTPS_TYPE_LOOKUP_H

#include "TypeLookupC.h"
#include <dds/DCPS/XTypes/TypeObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
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
