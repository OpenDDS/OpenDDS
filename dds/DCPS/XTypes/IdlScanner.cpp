/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "IdlScanner.h"

#  include "DynamicTypeImpl.h"

#  include <ace/Basic_Types.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

IdlToken
IdlScanner::scan_token(DDS::DynamicType_ptr type)
{
  OPENDDS_ASSERT(type);

  DDS::DynamicType_var bt = get_base_type(type);

  switch (bt->get_kind()) {
  case TK_BOOLEAN: {
    const IdlToken token = scan_identifier();
    if (token.is_boolean()) {
      return token;
    }
    break;
  }
  case TK_BYTE: {
    const IdlToken token = scan_numeric_literal();
    if (token.is_integer() && !token.is_signed() && token.integer_value() <= ACE_UINT8_MAX) {
      return token;
    }
    break;
  }
  case TK_INT8: {
    const IdlToken token = scan_numeric_literal();
    const ACE_UINT64 limit = static_cast<ACE_UINT64>(ACE_INT8_MAX) + (token.is_signed() ? 1 : 0);
    if (token.is_integer() && token.integer_value() <= limit) {
      return token;
    }
    break;
  }
  case TK_INT16: {
    const IdlToken token = scan_numeric_literal();
    const ACE_UINT64 limit = static_cast<ACE_UINT64>(ACE_INT16_MAX) + (token.is_signed() ? 1 : 0);
    if (token.is_integer() && token.integer_value() <= limit) {
      return token;
    }
    break;
  }
  case TK_INT32: {
    const IdlToken token = scan_numeric_literal();
    const ACE_UINT64 limit = static_cast<ACE_UINT64>(ACE_INT32_MAX) + (token.is_signed() ? 1 : 0);
    if (token.is_integer() && token.integer_value() <= limit) {
      return token;
    }
    break;
  }
  case TK_INT64: {
    const IdlToken token = scan_numeric_literal();
    const ACE_UINT64 limit = static_cast<ACE_UINT64>(ACE_INT64_MAX) + (token.is_signed() ? 1 : 0);
    if (token.is_integer() && token.integer_value() <= limit) {
      return token;
    }
    break;
  }
  case TK_UINT8: {
    const IdlToken token = scan_numeric_literal();
    if (token.is_integer() && !token.is_signed() && token.integer_value() <= ACE_UINT8_MAX) {
      return token;
    }
    break;
  }
  case TK_UINT16: {
    const IdlToken token = scan_numeric_literal();
    if (token.is_integer() && !token.is_signed() && token.integer_value() <= ACE_UINT16_MAX) {
      return token;
    }
    break;
  }
  case TK_UINT32: {
    const IdlToken token = scan_numeric_literal();
    if (token.is_integer() && !token.is_signed() && token.integer_value() <= ACE_UINT32_MAX) {
      return token;
    }
    break;
  }
  case TK_UINT64: {
    const IdlToken token = scan_numeric_literal();
    if (token.is_integer() && !token.is_signed() && token.integer_value() <= ACE_UINT64_MAX) {
      return token;
    }
    break;
  }
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128: {
    const IdlToken token = scan_numeric_literal();
    // TODO: Bounds checking.
    if (token.is_float()) {
      return token;
    }
    break;
  }
  case TK_CHAR8: {
    if (scan_character_value(true)) {
      return IdlToken::make_character(character_literal_);
    }
    break;
  }
  case TK_CHAR16:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERRROR: IdlScanner::scan_token does not support wide characters\n"));
    break;
  case TK_STRING8: {
    if (scan_string_value(false)) {
      return IdlToken::make_string(string_literal_);
    }
    break;
  }
  case TK_STRING16:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERRROR: IdlScanner::scan_token does not support wide strings\n"));
    break;
  case TK_ENUM: {
    const IdlToken token = scan_identifier();
    DDS::DynamicTypeMember_var dtm;
    if (token.is_identifier() && bt->get_member_by_name(dtm, token.identifier_value().c_str()) == DDS::RETCODE_OK) {
      return token;
    }
    break;
  }
  default:
    break;
  }

  return IdlToken::make_error();
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
