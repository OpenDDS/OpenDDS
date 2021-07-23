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
  size_t mutable_running_total = 0;
  serialized_size_delimiter(encoding, size);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.type_ids);

  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_In& stru)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(encoding, total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  size_t size = 0;

  serialized_size(encoding, size, stru.type_ids);
  if (!strm.write_parameter_id(206790757, size)) {
    return false;
  }

  return strm << stru.type_ids;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_In& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  unsigned member_id;
  size_t field_size;
  while (true) {

    if (strm.rpos() - start_pos >= total_size) {
      return true;
    }

    bool must_understand = false;
    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {
      return false;
    }

    switch (member_id) {
    case 206790757:
      if (!(strm >> stru.type_ids)) {
        return false;
      }
      break;
    default:
      if (must_understand) {
        if (DCPS_debug_level >= 8) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) unknown must_understand field(%u) in OpenDDS::XTypes::TypeLookup_getTypes_In\n"), member_id));
        }
        return false;
      }
      strm.skip(field_size);
      break;
    }
  }
  return false;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypes_Out& stru)
{
  size_t mutable_running_total = 0;
  serialized_size_delimiter(encoding, size);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.types);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.complete_to_minimal);

  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Out& stru)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(encoding, total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  size_t size = 0;
  serialized_size(encoding, size, stru.types);
  if (!strm.write_parameter_id(41962193, size)) {
    return false;
  }
  if (!(strm << stru.types)) {
    return false;
  }

  size = 0;
  serialized_size(encoding, size, stru.complete_to_minimal);
  if (!strm.write_parameter_id(193881463, size)) {
    return false;
  }
  return strm << stru.complete_to_minimal;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Out& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  unsigned member_id;
  size_t field_size;
  while (true) {

    if (strm.rpos() - start_pos >= total_size) {
      return true;
    }

    bool must_understand = false;
    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {
      return false;
    }

    switch (member_id) {
    case 41962193:
      if (!(strm >> stru.types)) {
        return false;
      }
      break;
    case 193881463:
      if (!(strm >> stru.complete_to_minimal)) {
        return false;
      }
      break;
    default:
      if (must_understand) {
        if (DCPS_debug_level >= 8) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) unknown must_understand field(%u) in OpenDDS::XTypes::TypeLookup_getTypes_Out\n"), member_id));
        }
        return false;
      }
      strm.skip(field_size);
      break;
    }
  }
  return false;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypes_Result& uni)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, uni.return_code);
  switch (uni.return_code) {
  case 0:
    serialized_size(encoding, size, uni.result);
    break;
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypes_Result& uni)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(encoding, total_size, uni);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  if (!(strm << uni.return_code)) {
    return false;
  }

  switch (uni.return_code) {
  case 0:
    return strm << uni.result;
  }

  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypes_Result& uni)
{
  size_t total_size;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  if (!(strm >> uni.return_code)) {
    return false;
  }

  switch (uni.return_code) {
  case 0:
    return strm >> uni.result;
  }

  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  size_t mutable_running_total = 0;
  serialized_size_delimiter(encoding, size);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.type_ids);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.continuation_point);

  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  size_t size = 0;
  serialized_size(encoding, size, stru.type_ids);
  if (!strm.write_parameter_id(206790757, size)) {
    return false;
  }
  if (!(strm << stru.type_ids)) {
    return false;
  }

  size = 0;
  serialized_size(encoding, size, stru.continuation_point);
  if (!strm.write_parameter_id(84468690, size)) {
    return false;
  }
  return strm << stru.continuation_point;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();
  unsigned member_id;
  size_t field_size;
  while (true) {

    if (strm.rpos() - start_pos >= total_size) {
      return true;
    }

    bool must_understand = false;
    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {
      return false;
    }

    switch (member_id) {
    case 206790757:
      if (!(strm >> stru.type_ids)) {
        return false;
      }
      break;
    case 84468690:
      if (!(strm >> stru.continuation_point)) {
        return false;
      }
      break;
    default:
      if (must_understand) {
        if (DCPS_debug_level >= 8) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) unknown must_understand field(%u) in OpenDDS::XTypes::TypeLookup_getTypeDependencies_In\n"), member_id));
        }
        return false;
      }
      strm.skip(field_size);
      break;
    }
  }
  return false;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_Out& stru)
{
  size_t mutable_running_total = 0;
  serialized_size_delimiter(encoding, size);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.dependent_typeids);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.continuation_point);

  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Out& stru)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  size_t size = 0;
  serialized_size(encoding, size, stru.dependent_typeids);
  if (!strm.write_parameter_id(195354569, size)) {
    return false;
  }
  if (!(strm << stru.dependent_typeids)) {
    return false;
  }

  size = 0;
  serialized_size(encoding, size, stru.continuation_point);
  if (!strm.write_parameter_id(84468690, size)) {
    return false;
  }
  return strm << stru.continuation_point;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Out& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();
  unsigned member_id;
  size_t field_size;

  while (true) {

    if (strm.rpos() - start_pos >= total_size) {
      return true;
    }

    bool must_understand = false;
    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {
      return false;
    }

    switch (member_id) {
    case 195354569:
      if (!(strm >> stru.dependent_typeids)) {
        return false;
      }
      break;
    case 84468690:
      if (!(strm >> stru.continuation_point)) {
        return false;
      }
      break;
    default:
      if (must_understand) {
        if (DCPS_debug_level >= 8) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) unknown must_understand field(%u) in OpenDDS::XTypes::TypeLookup_getTypeDependencies_Out\n"), member_id));
        }
        return false;
      }
      strm.skip(field_size);
      break;
    }
  }
  return false;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_getTypeDependencies_Result& uni)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, uni.return_code);

  switch (uni.return_code) {
  case 0:
    serialized_size(encoding, size, uni.result);
    break;
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_Result& uni)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(encoding, total_size, uni);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  if (!(strm << uni.return_code)) {
    return false;
  }

  switch (uni.return_code) {
  case 0:
    return strm << uni.result;
  }

  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_Result& uni)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  if (!(strm >> uni.return_code)) {
    return false;
  }

  switch (uni.return_code) {
  case 0:
    return strm >> uni.result;
  }

  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Call& uni)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, uni.kind);
  switch (uni.kind) {
  case 25318099:
    serialized_size(encoding, size, uni.getTypes);
    break;
  case 95091505:
    serialized_size(encoding, size, uni.getTypeDependencies);
    break;
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_Call& uni)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, uni);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  if (!(strm << uni.kind)) {
    return false;
  }

  switch (uni.kind) {
  case 25318099:
    return strm << uni.getTypes;
  case 95091505:
    return strm << uni.getTypeDependencies;
  }

  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_Call& uni)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  if (!(strm >> uni.kind)) {
    return false;
  }

  switch (uni.kind) {
  case 25318099:
    return strm >> uni.getTypes;
  case 95091505:
    return strm >> uni.getTypeDependencies;
  }

  return true;
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
  const XTypes::TypeLookup_Return& uni)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, uni.kind);

  switch (uni.kind) {
  case 25318099:
    serialized_size(encoding, size, uni.getType);
    break;
  case 95091505:
    serialized_size(encoding, size, uni.getTypeDependencies);
    break;
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_Return& uni)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, uni);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  if (!(strm << uni.kind)) {
    return false;
  }

  switch (uni.kind) {
  case 25318099:
    return strm << uni.getType;
  case 95091505:
    return strm << uni.getTypeDependencies;
  }

  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_Return& uni)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  if (!(strm >> uni.kind)) {
    return false;
  }

  switch (uni.kind) {
  case 25318099:
    return strm >> uni.getType;
  case 95091505:
    return strm >> uni.getTypeDependencies;
  }

  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeLookup_Reply& stru)
{
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru._cxx_return);
}

bool operator<<(Serializer& strm, const XTypes::TypeLookup_Reply& stru)
{
  return (strm << stru.header)
    && (strm << stru._cxx_return);
}

bool operator>>(Serializer& strm, XTypes::TypeLookup_Reply& stru)
{
  return (strm >> stru.header)
    && (strm >> stru._cxx_return);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::OctetSeq32& seq)
{
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_octet(encoding, size, seq.length());
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
