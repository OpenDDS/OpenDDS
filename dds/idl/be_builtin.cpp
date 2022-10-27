/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_builtin.h"
#include "be_util.h"
#include "../Version.h"

#include <global_extern.h>
#include <drv_extern.h>

#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <iomanip>

BE_BuiltinInterface::~BE_BuiltinInterface()
{
}

void
BE_BuiltinInterface::version() const
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(OPENDDS_VERSION)
             ACE_TEXT("\n")));
}

int
BE_BuiltinInterface::init(int&, ACE_TCHAR*[])
{
  ACE_NEW_RETURN(be_builtin_global, BE_BuiltinGlobalData, -1);
  idl_global->default_idl_version_ = IDL_VERSION_4;
  idl_global->anon_type_diagnostic(IDL_GlobalData::ANON_TYPE_SILENT);
  return 0;
}

void
BE_BuiltinInterface::post_init(char*[], long)
{
  std::ostringstream version;
  version << "-D__OPENDDS_IDL=0x"
          << std::setw(2) << std::setfill('0') << OPENDDS_MAJOR_VERSION
          << std::setw(2) << std::setfill('0') << OPENDDS_MINOR_VERSION
          << std::setw(2) << std::setfill('0') << OPENDDS_MICRO_VERSION;
  DRV_cpp_putarg(version.str().c_str());

#ifdef ACE_HAS_CDR_FIXED
  DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_FIXED");
#endif

  std::string include_dds = be_util::dds_root();
  if (include_dds.find(' ') != std::string::npos && include_dds[0] != '"') {
    include_dds.insert(include_dds.begin(), '"');
    include_dds.insert(include_dds.end(), '"');
  }
  be_builtin_global->add_inc_path(include_dds.c_str());
  ACE_CString included;
  DRV_add_include_path(included, include_dds.c_str(), 0, true);

  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_builtin_global->error("OpenDDS requires IDL version to be 4 or greater");
  } else {
    DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
    be_builtin_global->builtin_annotations_.register_all();
  }
}

// Clean up before exit, whether successful or not.
// Need not be exported since it is called only from this file.
void
BE_BuiltinInterface::cleanup()
{
  if (idl_global) {
    idl_global->destroy();
  }
}

void
BE_BuiltinInterface::destroy()
{
  be_builtin_global->destroy();
}

void BE_BuiltinInterface::parse_args(long& i, char** av)
{
  be_builtin_global->parse_args(i, av);
}
