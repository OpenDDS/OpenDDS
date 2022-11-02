
#include "windows_linking.h"

#if defined(ACE_WIN32)
#include <ace/SString.h>

namespace {
  typedef ACE_CString&
    (*add_include_path)(ACE_CString& include_path, const char* path,
                        const char* suffix, bool is_system);
  typedef void (*cpp_putarg)(const char* str);

  add_include_path add_include_path_ptr_ = 0;
  cpp_putarg cpp_putarg_ptr_ = 0;
}

opendds_idl_plugin_Export void set_drv_ptrs(void* inc, void* put)
{
  add_include_path_ptr_ = (add_include_path)inc;
  cpp_putarg_ptr_ = (cpp_putarg)put;
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
