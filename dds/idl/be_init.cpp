/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "../Version.h"

#include "tao/Version.h"
#include "global_extern.h"
#include "be_extern.h"
#include "drv_extern.h"

#include "ace/OS_NS_stdlib.h"

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

  const char* env = ACE_OS::getenv("DDS_ROOT");
  if (env && env[0]) {
    std::string dds_root = env;
    if (dds_root.find(' ') != std::string::npos && dds_root[0] != '"') {
      dds_root.insert(dds_root.begin(), '"');
      dds_root.insert(dds_root.end(), '"');
    }
    be_global->add_inc_path(dds_root.c_str());
    ACE_CString included;
    DRV_add_include_path(included, dds_root.c_str(), 0, true);
  }
}
