/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_extern.h"
#include "tao_idl_compat.h"
#include "../../../dds/Version.h"

#include <global_extern.h>
#include <drv_extern.h>

void
BE_version()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(OPENDDS_VERSION)
             ACE_TEXT("\n")));
}

int
BE_init(int &, ACE_TCHAR *[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
#ifdef TAO_IDL_HAS_IDL_VERSION_CLASS
  idl_global->default_idl_version_.version(IDL_VERSION_4);
#else
  idl_global->default_idl_version_ = IDL_VERSION_4;
#endif
#ifdef TAO_IDL_HAS_SCOPED_ANON_TYPE_DIAGNOSTIC
  idl_global->anon_type_diagnostic(IDL_GlobalData::Anon_Type_Diagnostic::Silent);
#else
  idl_global->anon_type_diagnostic(IDL_GlobalData::ANON_TYPE_SILENT);
#endif
  return 0;
}

void
#ifdef TAO_IDL_HAS_STD_DRV_FILES
BE_post_init(const std::vector<std::string>&)
#else
BE_post_init(char *[], long)
#endif
{
  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_global->error("OpenDDS requires IDL version to be 4 or greater");
    return;
  }

  idl_global->unknown_annotations_ = IDL_GlobalData::UNKNOWN_ANNOTATIONS_IGNORE;

  DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
  DRV_cpp_putarg("-DOPENDDS_HIDE_DYNAMIC_DATA");

  // If adding or changing annotations here, they should also be copied into
  // opendds_idl. dummy is there because IDL annotations can't be used before a
  // scoped name starting with :: unless they have annotation parameters.
  idl_global->eval(
    "module OpenDDS {module internal {@annotation hidden_op_in_java {int8 dummy;};};};\n");
}
