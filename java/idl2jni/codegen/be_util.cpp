
//=============================================================================
/**
 *  @file    be_util.cpp
 *
 *
 *  Static helper methods used by multiple visitors.
 *
 *
 *  @author Gary Maxey
 *  @author Jeff Parsons
 */
//=============================================================================

#include "be_util.h"
#include "be_extern.h"

#include "ast_generator.h"

#include "ace/OS_NS_strings.h"

// Prepare an argument for a BE
void
be_util::prep_be_arg (char *arg)
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
    be_global->stub_export_macro(arg + SZ_WB_STUB_EXPORT_MACRO);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_STUB_EXPORT_INCLUDE,
                                      SZ_WB_STUB_EXPORT_INCLUDE)) {
    be_global->stub_export_include(arg + SZ_WB_STUB_EXPORT_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_SKEL_EXPORT_MACRO,
                                      SZ_WB_SKEL_EXPORT_MACRO)) {
    be_global->skel_export_macro(arg + SZ_WB_SKEL_EXPORT_MACRO);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_SKEL_EXPORT_INCLUDE,
                                      SZ_WB_SKEL_EXPORT_INCLUDE)) {
    be_global->skel_export_include(arg + SZ_WB_SKEL_EXPORT_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_NATIVE_LIB_NAME,
                                      SZ_WB_NATIVE_LIB_NAME)) {
    be_global->native_lib_name(arg + SZ_WB_NATIVE_LIB_NAME);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_STUB_EXTRA_INCLUDE,
                                      SZ_WB_STUB_EXTRA_INCLUDE)) {
    be_global->add_include(arg + SZ_WB_STUB_EXTRA_INCLUDE,
                           BE_GlobalData::STUB_CPP);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_TAO_INC_PRE, SZ_WB_TAO_INC_PRE)) {
    be_global->tao_inc_pre_ = arg + SZ_WB_TAO_INC_PRE;
  }
}

void
be_util::arg_post_proc (void)
{
}

void
be_util::usage (void)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -o <dir>\t\tsets output directory for all files\n")
    ACE_TEXT(" -SI\t\t\tsuppress generation of *TypeSupport.idl\n")
    ACE_TEXT(" -Wb,export_macro=<macro name>\t\tsets export macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,export_include=<include path>\tsets export include ")
    ACE_TEXT("file for all files\n")
    ACE_TEXT(" -Wb,pch_include=<include path>\t\tsets include ")
    ACE_TEXT("file for precompiled header mechanism\n")
    ACE_TEXT(" -Wb,java[=<output_file>]\t\tenables Java support ")
    ACE_TEXT("for TypeSupport files.  Do not specify an 'output_file' ")
    ACE_TEXT("except for special cases.\n")
    ACE_TEXT(" -Wb,tao_include_prefix=<path>\t\tPrefix for including the TAO-")
    ACE_TEXT("generated header file.\n")
  ));
}

AST_Generator *
be_util::generator_init (void)
{
  AST_Generator* gen = 0;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

