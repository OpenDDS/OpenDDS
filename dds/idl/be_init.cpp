/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_extern.h"
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
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(DDS_VERSION)
             ACE_TEXT("\n")));
}

int
BE_init(int&, ACE_TCHAR*[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
  idl_global->default_idl_version_ = IDL_VERSION_4;
  return 0;
}

void
BE_post_init(char*[], long)
{
  std::ostringstream version;
  version << "-D__OPENDDS_IDL=0x"
          << std::setw(2) << std::setfill('0') << DDS_MAJOR_VERSION
          << std::setw(2) << std::setfill('0') << DDS_MINOR_VERSION
          << std::setw(2) << std::setfill('0') << DDS_MICRO_VERSION;
  DRV_cpp_putarg(version.str().c_str());

#ifdef ACE_HAS_CDR_FIXED
  DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_FIXED");
#endif

  std::string include_dds = be_util::dds_root();
  if (include_dds.find(' ') != std::string::npos && include_dds[0] != '"') {
    include_dds.insert(include_dds.begin(), '"');
    include_dds.insert(include_dds.end(), '"');
  }
  be_global->add_inc_path(include_dds.c_str());
  ACE_CString included;
  DRV_add_include_path(included, include_dds.c_str(), 0, true);

  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_global->error("OpenDDS requires IDL version to be 4 or greater");
  } else {
    DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
    be_global->builtin_annotations_.register_all();
  }
}
