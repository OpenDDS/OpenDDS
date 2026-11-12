/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_TAO_IDL_COMPAT_H
#define OPENDDS_IDL_TAO_IDL_COMPAT_H

#include <idl_defines.h>
#include <idl_global.h>
#include <ast_expression.h>

#include <ace/ACE.h>
#include <ace/SString.h>
#include <utl_string.h>

#include <string>
#include <vector>

namespace tao_idl_compat {

#ifdef TAO_IDL_HAS_STD_STRING_API
inline const char* c_str(const std::string& value)
{
  return value.c_str();
}

inline const char* str_value(const AST_Expression::AST_ExprValue& value)
{
  return value.strval.c_str();
}

inline const char* wstr_value(const AST_Expression::AST_ExprValue& value)
{
  return value.wstrval.c_str();
}

inline const IDL_GlobalData::DCPS_Key_List&
dcps_keys(const IDL_GlobalData::DCPS_Data_Type_Info& info)
{
  return info.key_list_;
}
#else
inline const char* c_str(const char* value)
{
  return value;
}

inline const char* c_str(const UTL_String* value)
{
  return value->get_string();
}

inline const char* c_str(const ACE_CString& value)
{
  return value.c_str();
}

inline const char* str_value(const AST_Expression::AST_ExprValue& value)
{
  return value.u.strval->get_string();
}

inline const char* wstr_value(const AST_Expression::AST_ExprValue& value)
{
  return value.u.wstrval;
}

inline std::vector<std::string>
dcps_keys(IDL_GlobalData::DCPS_Data_Type_Info& info)
{
  std::vector<std::string> keys;
  IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info.key_list_);
  for (ACE_TString* key = 0; iter.next(key) != 0; iter.advance()) {
    keys.push_back(ACE_TEXT_ALWAYS_CHAR(key->c_str()));
  }
  return keys;
}
#endif

} // namespace tao_idl_compat

#endif // OPENDDS_IDL_TAO_IDL_COMPAT_H
