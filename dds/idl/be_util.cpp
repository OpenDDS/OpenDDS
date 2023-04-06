
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

#include <ast_generator.h>

#include <ace/OS_NS_strings.h>

// Prepare an argument for a BE
void
be_util::prep_be_arg(char* arg)
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
  static const char WB_CPP_INCLUDE[] = "cpp_include=";
  static const size_t SZ_WB_CPP_INCLUDE = sizeof(WB_CPP_INCLUDE) - 1;
  static const char WB_JAVA[] = "java";
  static const size_t SZ_WB_JAVA = sizeof(WB_JAVA) - 1;
  static const char WB_TAO_INC_PRE[] = "tao_include_prefix=";
  static const size_t SZ_WB_TAO_INC_PRE = sizeof(WB_TAO_INC_PRE) - 1;
  static const char WB_TS_CPP_INCLUDE[] = "ts_cpp_include=";
  static const size_t SZ_WB_TS_CPP_INCLUDE = sizeof(WB_TS_CPP_INCLUDE) - 1;
  static const char WB_DDS_SEQ_SUFFIX[] = "opendds_sequence_suffix=";
  static const size_t SZ_WB_DDS_SEQ_SUFFIX = sizeof(WB_DDS_SEQ_SUFFIX) - 1;

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

  } else if (0 == ACE_OS::strncasecmp(arg, WB_CPP_INCLUDE, SZ_WB_CPP_INCLUDE)) {
    be_global->add_cpp_include(arg + SZ_WB_CPP_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_JAVA, SZ_WB_JAVA)) {
    be_global->java(true);
    if (ACE_OS::strlen(arg + SZ_WB_JAVA)) {
      be_global->java_arg(arg + SZ_WB_JAVA + 1 /* = */);
    }

  } else if (0 == ACE_OS::strncasecmp(arg, WB_TAO_INC_PRE, SZ_WB_TAO_INC_PRE)) {
    be_global->tao_inc_pre_ = arg + SZ_WB_TAO_INC_PRE;

  } else if (0 == ACE_OS::strncasecmp(arg, WB_TS_CPP_INCLUDE, SZ_WB_TS_CPP_INCLUDE)) {
    be_global->add_include(arg + SZ_WB_TS_CPP_INCLUDE, BE_GlobalData::STREAM_CPP);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_DDS_SEQ_SUFFIX, SZ_WB_DDS_SEQ_SUFFIX)) {
    be_global->sequence_suffix(arg + SZ_WB_DDS_SEQ_SUFFIX);
  }
}

void
be_util::arg_post_proc()
{
}

void
be_util::usage()
{
  // see be_global.cpp parse_args()
  ACE_DEBUG((LM_DEBUG,
    " -o <dir>               set output directory for all files\n"
    " -Lface                 generate FACE IDL to C++ mapping\n"
    " -Lspcpp                generate Safety Profile IDL to C++ mapping\n"
    " -Lc++11                generate IDL to C++11 mapping\n"
    " -SI                    suppress generation of *TypeSupport.idl\n"
    " -Sa                    suppress IDL any (ignored, for tao_idl compatibility)\n"
    " -St                    suppress IDL typecode when -L* option is present\n"
    " -Sv                    suppress ValueReader and ValueWriter generation\n"
    " -Sx                    suppress XTypes TypeObject and TypeIdentifier generation\n"
    " -Sdefault              exclude default TypeSupport generators from output\n"
    " -Gitl                  generate ITL\n"
    " -GfaceTS               generate FACE TS API for DCPS data types\n"
    " -Grapidjson            generate TypeSupport for converting data samples to\n"
    "                        RapidJSON JavaScript objects\n"
    " -Gxtypes-complete      generate XTypes complete TypeObject and TypeIdentifier\n"
    " --filename-only-includes                strip leading directories from generated\n"
    "                                         #include lines\n"
    " --[no-]default-nested                   topic types must be declared\n"
    "                                         (true by default)\n"
    " --no-dcps-data-type-warnings            don't warn about #pragma DCPS_DATA_TYPE\n"
    " --default-extensibility final|appendable|mutable\n"
    "                                         set the default XTypes extensibility\n"
    "                                         kind (appendable if not set)\n"
    " --default-autoid sequential|hash        set the default XTypes autoid approach\n"
    "                                         (sequential if not set)\n"
    " --default-try-construct discard|use-default|trim\n"
    "                                         set the default XTypes try-construct\n"
    "                                         behavior (discard if not set)\n"
    " --old-typeobject-encoding               use the pre-3.18 encoding of TypeObjects\n"
    "                                         when deriving TypeIdentifiers\n"
    " --old-typeobject-member-order           use the pre-3.24 struct and union\n"
    "                                         member order for TypeObjects, which is\n"
    "                                         ordered by member id instead of\n"
    "                                         declared order\n"
    " -Wb,export_macro=<macro name>           set export macro for all files\n"
    " --export=<macro name>                   Alias for -Wb,export_macro\n"
    " -Wb,export_include=<include path>       set export include file for all files\n"
    " -Wb,versioning_name=<macro name>        set versioning name macro for all files\n"
    " -Wb,versioning_begin=<macro name>       set versioning begin macro for all files\n"
    " -Wb,versioning_end=<macro name>         set versioning end macro for all files\n"
    " -Wb,pch_include=<include path>          set include file for precompiled header\n"
    "                                         mechanism\n"
    " -Wb,cpp_include=<include path>          set additional include file for cpp\n"
    "                                         files. Useful for 'after the fact'\n"
    "                                         typesupport generation with different\n"
    "                                         features enabled than in the first pass.\n"
    " -Wb,java[=<output_file>]                enable Java support for TypeSupport\n"
    "                                         files. Do not specify an 'output_file'\n"
    "                                         except for special cases.\n"
    " -Wb,tao_include_prefix=<path>           prefix for including the TAO-generated\n"
    "                                         header file.\n"
    " -Wb,ts_cpp_include=<include>            add <include> to *TypeSupportImpl.cpp\n"
    " -Wb,opendds_sequence_suffix=<suffix>    set the implied DDS sequence suffix\n"
    "                                         (default is 'Seq')\n"
    ));
}

AST_Generator*
be_util::generator_init()
{
  AST_Generator* gen = 0;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

const char*
be_util::dds_root()
{
  static const char* value = ACE_OS::getenv("DDS_ROOT");
  if (!value || !value[0]) {
    ACE_ERROR((LM_ERROR, "Error - The environment variable DDS_ROOT must be set.\n"));
    BE_abort();
  }
  return value;
}

void
be_util::misc_error_and_abort(const std::string& message, AST_Decl* node)
{
  idl_global->err()->misc_error(message.c_str(), node);
  BE_abort();
}
