#include "visibility_linking.h"

#if defined(ACE_Proper_Export_Flag) && \
    defined (OPENDDS_IDL_PLUGIN_HAS_DLL) && (OPENDDS_IDL_PLUGIN_HAS_DLL == 1)

namespace {
  add_include_path add_include_path_ptr_ = 0;
  cpp_putarg cpp_putarg_ptr_ = 0;
}

opendds_idl_plugin_Export void set_drv_ptrs(add_include_path inc_func,
                                            cpp_putarg put_func)
{
  add_include_path_ptr_ = inc_func;
  cpp_putarg_ptr_ = put_func;
}

opendds_idl_plugin_Export ACE_CString&
DRV_add_include_path(ACE_CString& include_path, const char* path,
                     const char* suffix, bool is_system)
{
  return add_include_path_ptr_(include_path, path, suffix, is_system);
}

opendds_idl_plugin_Export void DRV_cpp_putarg(const char* str)
{
  cpp_putarg_ptr_(str);
}
#endif
