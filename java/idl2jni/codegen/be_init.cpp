/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_extern.h"
#include "../../../dds/Version.h"

#include <global_extern.h>
#include <drv_extern.h>

void
BE_version()
{
  const char* const version_metadata = "-" OPENDDS_VERSION_METADATA;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(OPENDDS_VERSION)
             ACE_TEXT("%C\n"), version_metadata[1] ? version_metadata : ""));
}

int
BE_init(int &, ACE_TCHAR *[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
  idl_global->default_idl_version_ = IDL_VERSION_4;
  idl_global->anon_type_diagnostic(IDL_GlobalData::ANON_TYPE_SILENT);
  return 0;
}

void
BE_post_init(char *[], long)
{
  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_global->error("OpenDDS requires IDL version to be 4 or greater");
  } else {
    DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
    idl_global->unknown_annotations_ = IDL_GlobalData::UNKNOWN_ANNOTATIONS_IGNORE;
  }
}
