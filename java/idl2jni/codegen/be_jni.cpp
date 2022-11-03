#include "be_jni.h"

#include "../../../dds/Version.h"
#include "idl2jni_export.h"

#include <drv_extern.h>
#include <global_extern.h>
#include <ace/OS_NS_strings.h>

namespace {
  BE_JNIInterface* instance = 0;
}

// This function is used to register our interface with the driver
extern "C" {
  idl2jni_Export BE_Interface* idl2jni_allocator()
  {
    if (instance == 0) {
      ACE_NEW_RETURN(instance, BE_JNIInterface, 0);
    }
    return instance;
  }
}

BE_JNIInterface::~BE_JNIInterface()
{
}

int BE_JNIInterface::init(int&, ACE_TCHAR* [])
{
  ACE_NEW_RETURN(be_jni_global, BE_JNIGlobalData, -1);
  idl_global->default_idl_version_ = IDL_VERSION_4;
  idl_global->anon_type_diagnostic(IDL_GlobalData::ANON_TYPE_SILENT);
  return 0;
}

void BE_JNIInterface::post_init(char* [], long)
{
  if (idl_global->idl_version_ < IDL_VERSION_4) {
    idl_global->ignore_files_ = true; // Exit without parsing files
    be_jni_global->error("OpenDDS requires IDL version to be 4 or greater");
  }
  else {
    DRV_cpp_putarg("-D__OPENDDS_IDL_HAS_ANNOTATIONS");
    idl_global->unknown_annotations_ = IDL_GlobalData::UNKNOWN_ANNOTATIONS_IGNORE;
  }
}

void BE_JNIInterface::version() const
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ")
                       ACE_TEXT(OPENDDS_VERSION) ACE_TEXT("\n")));
}

void BE_JNIInterface::cleanup()
{
  if (idl_global) {
    idl_global->destroy();
  }
}

void BE_JNIInterface::destroy()
{
  be_jni_global->destroy();
}

void BE_JNIInterface::parse_args(long& i, char** av)
{
  be_jni_global->parse_args(i, av);
}

void BE_JNIInterface::prep_be_arg(char* arg)
{
  static const char WB_STUB_EXPORT_MACRO[] = "stub_export_macro=";
  static const size_t SZ_WB_STUB_EXPORT_MACRO =
    sizeof(WB_STUB_EXPORT_MACRO) - 1;
  static const char WB_STUB_EXPORT_INCLUDE[] = "stub_export_include=";
  static const size_t SZ_WB_STUB_EXPORT_INCLUDE =
    sizeof(WB_STUB_EXPORT_INCLUDE) - 1;
  static const char WB_SKEL_EXPORT_MACRO[] = "skel_export_macro=";
  static const size_t SZ_WB_SKEL_EXPORT_MACRO =
    sizeof(WB_SKEL_EXPORT_MACRO) - 1;
  static const char WB_SKEL_EXPORT_INCLUDE[] = "skel_export_include=";
  static const size_t SZ_WB_SKEL_EXPORT_INCLUDE =
    sizeof(WB_SKEL_EXPORT_INCLUDE) - 1;
  static const char WB_NATIVE_LIB_NAME[] = "native_lib_name=";
  static const size_t SZ_WB_NATIVE_LIB_NAME =
    sizeof(WB_NATIVE_LIB_NAME) - 1;
  static const char WB_STUB_EXTRA_INCLUDE[] = "stub_extra_include=";
  static const size_t SZ_WB_STUB_EXTRA_INCLUDE =
    sizeof(WB_STUB_EXTRA_INCLUDE) - 1;
  static const char WB_TAO_INC_PRE[] = "tao_include_prefix=";
  static const size_t SZ_WB_TAO_INC_PRE = sizeof(WB_TAO_INC_PRE) - 1;

  if (0 == ACE_OS::strncasecmp(arg, WB_STUB_EXPORT_MACRO,
    SZ_WB_STUB_EXPORT_MACRO)) {
    be_jni_global->stub_export_macro(arg + SZ_WB_STUB_EXPORT_MACRO);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_STUB_EXPORT_INCLUDE,
    SZ_WB_STUB_EXPORT_INCLUDE)) {
    be_jni_global->stub_export_include(arg + SZ_WB_STUB_EXPORT_INCLUDE);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_SKEL_EXPORT_MACRO,
    SZ_WB_SKEL_EXPORT_MACRO)) {
    be_jni_global->skel_export_macro(arg + SZ_WB_SKEL_EXPORT_MACRO);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_SKEL_EXPORT_INCLUDE,
    SZ_WB_SKEL_EXPORT_INCLUDE)) {
    be_jni_global->skel_export_include(arg + SZ_WB_SKEL_EXPORT_INCLUDE);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_NATIVE_LIB_NAME,
    SZ_WB_NATIVE_LIB_NAME)) {
    be_jni_global->native_lib_name(arg + SZ_WB_NATIVE_LIB_NAME);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_STUB_EXTRA_INCLUDE,
    SZ_WB_STUB_EXTRA_INCLUDE)) {
    be_jni_global->add_include(arg + SZ_WB_STUB_EXTRA_INCLUDE,
      BE_JNIGlobalData::STUB_CPP);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_TAO_INC_PRE, SZ_WB_TAO_INC_PRE)) {
    be_jni_global->tao_inc_pre_ = arg + SZ_WB_TAO_INC_PRE;
  }
}

void BE_JNIInterface::arg_post_proc()
{
}

void BE_JNIInterface::usage()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -o <dir>\t\tsets output directory for all files\n")
    ACE_TEXT(" -SI\t\t\tsuppress generation of *TypeSupport.idl\n")
    ACE_TEXT(" -Wb,export_macro=<macro>\tsets export macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,export_include=<path>\tsets export include ")
    ACE_TEXT("file for all files\n")
    ACE_TEXT(" -Wb,pch_include=<path>\t\tsets include ")
    ACE_TEXT("file for precompiled header mechanism\n")
    ACE_TEXT(" -Wb,java[=<output_file>]\tenables Java support ")
    ACE_TEXT("for TypeSupport files.  Do\n\t\t\t\tnot specify an 'output_file' ")
    ACE_TEXT("except for special\n\t\t\t\tcases.\n")
    ACE_TEXT(" -Wb,tao_include_prefix=<path>\tPrefix for including the TAO-")
    ACE_TEXT("generated header file.\n")
    ));
}
