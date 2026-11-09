/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL2JNI_TAO_IDL_COMPAT_H
#define OPENDDS_IDL2JNI_TAO_IDL_COMPAT_H

#include <idl_defines.h>

#include <ace/SString.h>
#include <utl_string.h>

#include <string>

namespace tao_idl_compat {

#ifdef TAO_IDL_HAS_STD_STRING_API
inline const char* c_str(const std::string& value)
{
  return value.c_str();
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
#endif

} // namespace tao_idl_compat

#endif // OPENDDS_IDL2JNI_TAO_IDL_COMPAT_H
