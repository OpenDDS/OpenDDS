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
    serialized_size(encoding, size, stru.type_ids);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_In& stru)
  {
    return strm << stru.type_ids;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_In& stru)
  {
    return strm >> stru.type_ids;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypes_Out& stru)
  {
    serialized_size(encoding, size, stru.types);
    serialized_size(encoding, size, stru.complete_to_minimal);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Out& stru)
  {
    return (strm << stru.types)
      && (strm << stru.complete_to_minimal);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Out& stru)
  {
    return (strm >> stru.types)
      && (strm >> stru.complete_to_minimal);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypes_Result& stru)
  {
    serialized_size(encoding, size, stru.result);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Result& stru)
  {
    return strm << stru.result;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Result& stru)
  {
    return strm >> stru.result;
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::ContinuationPoint&)
  {
    max_serialized_size_octet(encoding, size, 32);
  }

  bool operator<<(Serializer& strm, const XTypes::ContinuationPoint& _tao_sequence)
  {
    return TAO::marshal_sequence(strm, _tao_sequence);
  }

  bool operator>>(Serializer& strm, XTypes::ContinuationPoint& _tao_sequence)
  {
    return TAO::demarshal_sequence(strm, _tao_sequence);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    serialized_size(encoding, size, stru.type_ids);
    max_serialized_size_octet(encoding, size, 32);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    return (strm << stru.type_ids)
      && (strm << stru.continuation_point);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru)
  {
    return (strm >> stru.type_ids)
      && (strm >> stru.continuation_point);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    serialized_size(encoding, size, stru.dependent_typeids);
    max_serialized_size_octet(encoding, size, 32);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    return (strm << stru.dependent_typeids)
      && (strm << stru.continuation_point);
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Out& stru)
  {
    return (strm >> stru.dependent_typeids)
      && (strm >> stru.continuation_point);
  }


  void serialized_size(const Encoding& encoding, size_t& size,
    const XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    serialized_size(encoding, size, stru.result);
  }

  bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    return strm << stru.result;
  }

  bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Result& stru)
  {
    return strm >> stru.result;
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
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
