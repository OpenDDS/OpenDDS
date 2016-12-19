
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
  static const char WB_EXPORT_MACRO[] = "export_macro=";
  static const size_t SZ_WB_EXPORT_MACRO = sizeof(WB_EXPORT_MACRO) - 1;
  static const char WB_EXPORT_INCLUDE[] = "export_include=";
  static const size_t SZ_WB_EXPORT_INCLUDE = sizeof(WB_EXPORT_INCLUDE) - 1;
  static const char WB_VERSIONING_NAME[] = "versioning_name=";
  static const size_t SZ_WB_VERSIONING_NAME = sizeof(WB_VERSIONING_NAME) - 1;
  static const char WB_VERSIONING_BEGIN[] = "versioning_begin=";
  static const size_t SZ_WB_VERSIONING_BEGIN = sizeof(WB_VERSIONING_BEGIN) - 1;
  static const char WB_VERSIONING_END[] = "versioning_end=";
  static const size_t SZ_WB_VERSIONING_END = sizeof(WB_VERSIONING_END) - 1;
  static const char WB_PCH_INCLUDE[] = "pch_include=";
  static const size_t SZ_WB_PCH_INCLUDE = sizeof(WB_PCH_INCLUDE) - 1;
  static const char WB_JAVA[] = "java";
  static const size_t SZ_WB_JAVA = sizeof(WB_JAVA) - 1;
  static const char WB_TAO_INC_PRE[] = "tao_include_prefix=";
  static const size_t SZ_WB_TAO_INC_PRE = sizeof(WB_TAO_INC_PRE) - 1;
  static const char WB_V8[] = "v8";
  static const size_t SZ_WB_V8 = sizeof(WB_V8) - 1;

  if (0 == ACE_OS::strncasecmp(arg, WB_EXPORT_MACRO, SZ_WB_EXPORT_MACRO)) {
    be_global->export_macro(arg + SZ_WB_EXPORT_MACRO);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_EXPORT_INCLUDE,
                                      SZ_WB_EXPORT_INCLUDE)) {
    be_global->export_include(arg + SZ_WB_EXPORT_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_NAME,
                                      SZ_WB_VERSIONING_NAME)) {
    be_global->versioning_name(arg + SZ_WB_VERSIONING_NAME);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_BEGIN,
                                      SZ_WB_VERSIONING_BEGIN)) {
    be_global->versioning_begin(arg + SZ_WB_VERSIONING_BEGIN);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_END,
                                      SZ_WB_VERSIONING_END)) {
    be_global->versioning_end(arg + SZ_WB_VERSIONING_END);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_PCH_INCLUDE, SZ_WB_PCH_INCLUDE)) {
    be_global->pch_include(arg + SZ_WB_PCH_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_JAVA, SZ_WB_JAVA)) {
    be_global->java(true);
    if (ACE_OS::strlen(arg + SZ_WB_JAVA)) {
      be_global->java_arg(arg + SZ_WB_JAVA + 1 /* = */);
    }

  } else if (0 == ACE_OS::strncasecmp(arg, WB_TAO_INC_PRE, SZ_WB_TAO_INC_PRE)) {
    be_global->tao_inc_pre_ = arg + SZ_WB_TAO_INC_PRE;

  } else if (0 == ACE_OS::strncasecmp(arg, WB_V8, SZ_WB_V8)) {
    be_global->v8(true);
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
    ACE_TEXT(" -Lface\t\t\tgenerate FACE IDL to C++ mapping\n")
    ACE_TEXT(" -Lspcpp\t\t\tgenerate Safety Profile IDL to C++ mapping\n")
    ACE_TEXT(" -SI\t\t\tsuppress generation of *TypeSupport.idl\n")
    ACE_TEXT(" -Sa\t\t\tsuppress IDL any (ignored, for tao_idl compatibility)\n")
    ACE_TEXT(" -St\t\t\tsuppress IDL typecode when -L* option is present\n")
    ACE_TEXT(" -Gws\t\t\tgenerate wireshark sample dissector configuration\n")
    ACE_TEXT(" -Gitl\t\t\tgenerate ITL\n")
    ACE_TEXT(" -GfaceTS\t\tgenerate FACE TS API for DCPS data types\n")
    ACE_TEXT(" -Wb,export_macro=<macro name>\t\tsets export macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,export_include=<include path>\tsets export include ")
    ACE_TEXT("file for all files\n")
    ACE_TEXT(" -Wb,versioning_name=<macro name>\tsets versioning name macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_begin=<macro name>\tsets versioning begin macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_end=<macro name>\tsets versioning end macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,pch_include=<include path>\t\tsets include ")
    ACE_TEXT("file for precompiled header mechanism\n")
    ACE_TEXT(" -Wb,java[=<output_file>]\t\tenables Java support ")
    ACE_TEXT("for TypeSupport files.  Do not specify an 'output_file' ")
    ACE_TEXT("except for special cases.\n")
    ACE_TEXT(" -Wb,tao_include_prefix=<path>\t\tPrefix for including the TAO-")
    ACE_TEXT("generated header file.\n")
    ACE_TEXT(" -Wb,v8\t\t\tgenerate TypeSupport for converting data samples ")
    ACE_TEXT("to v8 JavaScript objects\n")
    ACE_TEXT(" -ZC <include>\t\t\tadd <include> to *TypeSupportImpl.cpp\n")
  ));
}

AST_Generator *
be_util::generator_init (void)
{
  AST_Generator* gen = 0;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}
