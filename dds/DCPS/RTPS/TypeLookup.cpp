/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeLookup.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace DCPS {

  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypes_In& stru)
  {
    // TODO: needs correct implementation
    serialized_size(encoding, size, stru.type_ids);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_In& stru)
  {
    // TODO: needs correct implementation
    return strm << stru.type_ids;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_In& stru)
  {
    // TODO: needs correct implementation
    return strm >> stru.type_ids;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypes_Out& stru)
  {
    // TODO: needs correct implementation
    serialized_size(encoding, size, stru.types);
    serialized_size(encoding, size, stru.complete_to_minimal);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Out& stru)
  {
    // TODO: needs correct implementation
    return (strm << stru.types)
      && (strm << stru.complete_to_minimal);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Out& stru)
  {
    // TODO: needs correct implementation
    return (strm >> stru.types)
      && (strm >> stru.complete_to_minimal);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypes_Result& stru)
  {
    switch (stru.return_code) {
    case DDS::RETCODE_OK:
      serialized_size(encoding, size, stru.result);
      break;
    }
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Result& stru)
  {
    if (!(strm << ACE_OutputCDR::from_octet(stru.return_code))) {
      return false;
    }
    switch (stru.return_code) {
    case DDS::RETCODE_OK:
      return strm << stru.result;
    }
    return false;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Result& stru)
  {
    CORBA::ULong return_code;
    if (!(strm >> return_code)) {
      return false;
    }
    switch (return_code) {
    case DDS::RETCODE_OK: {
      OpenDDS::XTypes::TypeLookup_getTypes_Out tmp;
      if (strm >> tmp) {
        stru.result = tmp;
        stru.return_code = return_code;
        return true;
      }
      return false;
    }
    }
    return false;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    // TODO: needs correct implementation
    serialized_size(encoding, size, stru.type_ids);
    serialized_size(encoding, size, stru.continuation_point);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    // TODO: needs correct implementation
    return (strm << stru.type_ids)
      && (strm << stru.continuation_point);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    // TODO: needs correct implementation
    return (strm >> stru.type_ids)
      && (strm >> stru.continuation_point);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    // TODO: needs correct implementation
    serialized_size(encoding, size, stru.dependent_typeids);
    serialized_size(encoding, size, stru.continuation_point);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    // TODO: needs correct implementation
    return (strm << stru.dependent_typeids)
      && (strm << stru.continuation_point);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    // TODO: needs correct implementation
    return (strm >> stru.dependent_typeids)
      && (strm >> stru.continuation_point);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    switch (stru.return_code) {
    case DDS::RETCODE_OK:
      serialized_size(encoding, size, stru.result);
      break;
    }
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    if (!(strm << ACE_OutputCDR::from_octet(stru.return_code))) {
      return false;
    }
    switch (stru.return_code) {
    case DDS::RETCODE_OK:
      return strm << stru.result;
    }
    return false;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    CORBA::ULong return_code;
    if (!(strm >> return_code)) {
      return false;
    }
    switch (return_code) {
    case DDS::RETCODE_OK: {
      OpenDDS::XTypes::TypeLookup_getTypeDependencies_Out tmp;
      if (strm >> tmp) {
        stru.result = tmp;
        stru.return_code = return_code;
        return true;
      }
      return false;
    }
    }
    return false;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_Call& stru)
  {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.kind));
    switch (stru.kind) {
    case XTypes::TypeLookup_getTypes_HashId:
      serialized_size(encoding, size, stru.getTypes);
      break;
    case XTypes::TypeLookup_getDependencies_HashId:
      serialized_size(encoding, size, stru.getTypeDependencies);
      break;
    }
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_Call& stru)
  {
    if (!(strm << ACE_OutputCDR::from_octet(stru.kind))) {
      return false;
    }
    switch (stru.kind) {
    case XTypes::TypeLookup_getTypes_HashId:
      return (strm << stru.getTypes);
    case XTypes::TypeLookup_getDependencies_HashId:
      return (strm << stru.getTypeDependencies);
    }
    return false;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_Call& stru)
  {
    ACE_CDR::Octet kind;
    if (!(strm >> ACE_InputCDR::to_octet(kind))) {
      return false;
    }
    switch (kind) {
    case XTypes::TypeLookup_getTypes_HashId: {
      OpenDDS::XTypes::TypeLookup_getTypes_In tmp;
      if (strm >> tmp) {
        stru.getTypes = tmp;
        stru.kind = kind;
        return true;
      }
      return false;
    }
    case XTypes::TypeLookup_getDependencies_HashId: {
      OpenDDS::XTypes::TypeLookup_getTypeDependencies_In tmp;
      if (strm >> tmp) {
        stru.getTypeDependencies = tmp;
        stru.kind = kind;
        return true;
      }
      return false;
    }
    }
    return false;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_Request& stru)
  {
    serialized_size(encoding, size, stru.header);
    serialized_size(encoding, size, stru.data);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_Request& stru)
  {
    return (strm << stru.header)
      && (strm << stru.data);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_Request& stru)
  {
    return (strm >> stru.header)
      && (strm >> stru.data);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_Return& stru)
  {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.kind));
    switch (stru.kind) {
    case XTypes::TypeLookup_getTypes_HashId: {
      serialized_size(encoding, size, stru.getTypes);
      break;
    }
    case XTypes::TypeLookup_getDependencies_HashId: {
      serialized_size(encoding, size, stru.getTypeDependencies);
      break;
    }
    }
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_Return& stru)
  {
    if (!(strm << ACE_OutputCDR::from_octet(stru.kind))) {
      return false;
    }
    switch (stru.kind) {
    case XTypes::TypeLookup_getTypes_HashId: {
      return (strm << stru.getTypes);
    }
    case XTypes::TypeLookup_getDependencies_HashId: {
      return (strm << stru.getTypeDependencies);
    }
    }
    return false;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_Return& stru)
  {
    ACE_CDR::Octet kind;
    if (!(strm >> ACE_InputCDR::to_octet(kind))) {
      return false;
    }
    switch (kind) {
    case XTypes::TypeLookup_getTypes_HashId: {
      OpenDDS::XTypes::TypeLookup_getTypes_Result tmp;
      if (strm >> tmp) {
        stru.getTypes = tmp;
        stru.kind = kind;
        return true;
      }
      return false;
    }
    case XTypes::TypeLookup_getDependencies_HashId: {
      OpenDDS::XTypes::TypeLookup_getTypeDependencies_Result tmp;
      if (strm >> tmp) {
        stru.getTypeDependencies = tmp;
        stru.kind = kind;
        return true;
      }
      return false;
    }
    }
    return false;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_Reply& stru)
  {
    serialized_size(encoding, size, stru.header);
    serialized_size(encoding, size, stru.data);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_Reply& stru)
  {
    return (strm << stru.header)
      && (strm << stru.data);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_Reply& stru)
  {
    return (strm >> stru.header)
      && (strm >> stru.data);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::OctetSeq32& seq)
  {
    DCPS::serialized_size_ulong(encoding, size);
    if (seq.length() == 0) {
      return;
    }
    max_serialized_size_octet(encoding, size, seq.length());
  }

  bool operator<<(Serializer& strm, const XTypes::OctetSeq32& seq)
  {
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

  bool operator>>(Serializer& strm, XTypes::OctetSeq32& seq)
  {
    CORBA::ULong length;
    if (!(strm >> length)) {
      return false;
    }
    if (length > 32) {
      return false;
    }
    seq.length(length);
    if (length == 0) {
      return true;
    }
    return strm.read_octet_array(seq.get_buffer(), length);
  }

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
