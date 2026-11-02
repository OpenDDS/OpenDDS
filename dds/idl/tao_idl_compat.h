/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TAO_IDL_COMPAT_H
#define OPENDDS_TAO_IDL_COMPAT_H

#include <idl_defines.h>
#include <idl_global.h>

#include <ace/ACE.h>
#include <ace/SString.h>
#include <utl_string.h>

#include <string>
#include <vector>

namespace tao_idl_compat {

#ifdef TAO_IDL_HAS_STD_STRING_API
inline char const *c_str (std::string const& value)
{
  return value.c_str ();
}
#else
inline char const *c_str (char const *value)
{
  return value;
}

inline char const *c_str (UTL_String const *value)
{
  return value->get_string ();
}

inline char const *c_str (ACE_CString const& value)
{
  return value.c_str ();
}
#endif

inline std::vector<std::string>
dcps_keys (IDL_GlobalData::DCPS_Data_Type_Info& info)
{
#ifdef TAO_IDL_HAS_STD_DCPS_DATA_TYPE_INFO
  return info.key_list_;
#else
  std::vector<std::string> keys;
  IDL_GlobalData::DCPS_Data_Type_Info_Iter iter (info.key_list_);
  for (ACE_TString *key = 0; iter.next (key) != 0; iter.advance ())
    {
      keys.push_back (ACE_TEXT_ALWAYS_CHAR (key->c_str ()));
    }
  return keys;
#endif
}

} // namespace tao_idl_compat

#endif // OPENDDS_TAO_IDL_COMPAT_H
