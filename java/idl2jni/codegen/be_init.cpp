/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tao/Version.h"

#include "global_extern.h"
#include "be_extern.h"
#include "drv_extern.h"

void
BE_version()
{
  ACE_DEBUG((LM_DEBUG,
             "%s %s\n",
             ACE_TEXT("IDL2JNI_BE, version"),
             ACE_TEXT(TAO_VERSION)));
}

int
BE_init(int &, ACE_TCHAR *[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
  idl_global->default_idl_version_ = IDL_VERSION_4;
  return 0;
}

void
BE_post_init(char *[], long)
{
  if (idl_global->idl_version_ >= IDL_VERSION_4) {
    idl_global->unknown_annotations_ = IDL_GlobalData::UNKNOWN_ANNOTATIONS_IGNORE;
    DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
  }
}
