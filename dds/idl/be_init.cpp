/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_extern.h"
#include "tao_idl_compat.h"
#include "be_util.h"
#include "../Version.h"

#include <global_extern.h>
#include <drv_extern.h>

#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <iomanip>

void
BE_version()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(OPENDDS_VERSION)
             ACE_TEXT("\n")));
}

int
BE_init(int&, ACE_TCHAR*[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
#ifdef TAO_IDL_HAS_IDL_VERSION_CLASS
  idl_global->default_idl_version_.version (IDL_VERSION_4);
#else
  idl_global->default_idl_version_ = IDL_VERSION_4;
#endif
#ifdef TAO_IDL_HAS_SCOPED_ANON_TYPE_DIAGNOSTIC
  idl_global->anon_type_diagnostic (IDL_GlobalData::Anon_Type_Diagnostic::Silent);
#else
  idl_global->anon_type_diagnostic (IDL_GlobalData::ANON_TYPE_SILENT);
#endif
  return 0;
}

void
#ifdef TAO_IDL_HAS_STD_DRV_FILES
BE_post_init(std::vector<std::string> const&)
#else
BE_post_init(char*[], long)
#endif
{
  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_global->error("OpenDDS requires IDL version to be 4 or greater");
    return;
  }

  std::ostringstream version;
  version << "-D__OPENDDS_IDL=0x"
          << std::setw(2) << std::setfill('0') << OPENDDS_MAJOR_VERSION
          << std::setw(2) << std::setfill('0') << OPENDDS_MINOR_VERSION
          << std::setw(2) << std::setfill('0') << OPENDDS_MICRO_VERSION;
  DRV_cpp_putarg(version.str().c_str());

  DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_FIXED");

  std::string include_dds = be_util::dds_root();
  if (include_dds.find(' ') != std::string::npos && include_dds[0] != '"') {
    include_dds.insert(include_dds.begin(), '"');
    include_dds.insert(include_dds.end(), '"');
  }
  be_global->add_inc_path(include_dds.c_str());
#ifdef TAO_IDL_HAS_STD_STRING_API
  std::string included;
  DRV_add_include_path (included, include_dds, std::string (), true);
#else
  ACE_CString included;
  DRV_add_include_path (included, include_dds.c_str (), 0, true);
#endif

  DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
  be_global->builtin_annotations_.register_all();
  // This annotation isn't used, but must match the one in idl2jni to avoid
  // warnings or errors.
  idl_global->eval(
    "module OpenDDS {module internal {@annotation hidden_op_in_java {int8 dummy;};};};\n");
}
