/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_builtin.h"
#include "be_util.h"
#include "../Version.h"

#include "cxx11_language_mapping.h"
#include "sp_language_mapping.h"

#include <global_extern.h>
#include <drv_extern.h>

#include <ace/DLL_Manager.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_strings.h>

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
BE_BuiltinInterface::init(int& argc, ACE_TCHAR* argv[])
{
  ACE_NEW_RETURN(be_builtin_global, BE_BuiltinGlobalData, -1);
  allocate_language_mapping(argc, argv);
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

void BE_BuiltinInterface::prep_be_arg(char* arg)
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
    be_builtin_global->export_macro(arg + SZ_WB_EXPORT_MACRO);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_EXPORT_INCLUDE,
    SZ_WB_EXPORT_INCLUDE)) {
    be_builtin_global->export_include(arg + SZ_WB_EXPORT_INCLUDE);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_NAME,
    SZ_WB_VERSIONING_NAME)) {
    be_builtin_global->versioning_name(arg + SZ_WB_VERSIONING_NAME);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_BEGIN,
    SZ_WB_VERSIONING_BEGIN)) {
    be_builtin_global->versioning_begin(arg + SZ_WB_VERSIONING_BEGIN);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_VERSIONING_END,
    SZ_WB_VERSIONING_END)) {
    be_builtin_global->versioning_end(arg + SZ_WB_VERSIONING_END);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_PCH_INCLUDE, SZ_WB_PCH_INCLUDE)) {
    be_builtin_global->pch_include(arg + SZ_WB_PCH_INCLUDE);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_CPP_INCLUDE, SZ_WB_CPP_INCLUDE)) {
    be_builtin_global->add_cpp_include(arg + SZ_WB_CPP_INCLUDE);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_JAVA, SZ_WB_JAVA)) {
    be_builtin_global->java(true);
    if (ACE_OS::strlen(arg + SZ_WB_JAVA)) {
      be_builtin_global->java_arg(arg + SZ_WB_JAVA + 1 /* = */);
    }

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_TAO_INC_PRE, SZ_WB_TAO_INC_PRE)) {
    be_builtin_global->tao_inc_pre_ = arg + SZ_WB_TAO_INC_PRE;

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_TS_CPP_INCLUDE, SZ_WB_TS_CPP_INCLUDE)) {
    be_builtin_global->add_include(arg + SZ_WB_TS_CPP_INCLUDE, BE_BuiltinGlobalData::STREAM_CPP);

  }
  else if (0 == ACE_OS::strncasecmp(arg, WB_DDS_SEQ_SUFFIX, SZ_WB_DDS_SEQ_SUFFIX)) {
    be_builtin_global->sequence_suffix(arg + SZ_WB_DDS_SEQ_SUFFIX);
  }
}

void BE_BuiltinInterface::arg_post_proc()
{
}

void BE_BuiltinInterface::usage()
{
  // see be_global.cpp parse_args()
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -o <dir>\t\tset output directory for all files\n")
    ACE_TEXT(" -Lface\t\t\tgenerate FACE IDL to C++ mapping\n")
    ACE_TEXT(" -Lspcpp\t\tgenerate Safety Profile IDL to C++ mapping\n")
    ACE_TEXT(" -Lc++11\t\tgenerate IDL to C++11 mapping\n")
    ACE_TEXT(" -SI\t\t\tsuppress generation of *TypeSupport.idl\n")
    ACE_TEXT(" -Sa\t\t\tsuppress IDL any (ignored, for tao_idl compatibility)\n")
    ACE_TEXT(" -St\t\t\tsuppress IDL typecode when -L* option is present\n")
    ACE_TEXT(" -Sv\t\t\tsuppress ValueReader and ValueWriter generation\n")
    ACE_TEXT(" -Sx\t\t\tsuppress XTypes TypeObject and TypeIdentifier generation\n")
    ACE_TEXT(" -Sdefault\t\texclude default TypeSupport generators from output\n")
    ACE_TEXT(" -Gitl\t\t\tgenerate ITL\n")
    ACE_TEXT(" -GfaceTS\t\tgenerate FACE TS API for DCPS data types\n")
    ACE_TEXT(" -Grapidjson\t\tgenerate TypeSupport for converting data samples ")
    ACE_TEXT("to RapidJSON JavaScript objects\n")
    ACE_TEXT(" -Gxtypes-complete\t\t\tgenerate XTypes complete TypeObject and TypeIdentifier\n")
    ACE_TEXT(" --filename-only-includes\tStrip leading directories from generated #include lines\n")
    ACE_TEXT(" --[no-]default-nested\ttopic types must be declared, true by default\n")
    ACE_TEXT(" --no-dcps-data-type-warnings\t\tdon't warn about #pragma DCPS_DATA_TYPE\n")
    ACE_TEXT(" --default-extensibility\t\tset default XTypes extensibility kind to final, ")
    ACE_TEXT("appendable, or mutable\n\t\t\t\t\t(appendable if not set)\n")
    ACE_TEXT(" --default-autoid\t\t\tset default XTypes autoid approach to sequential or hash\n")
    ACE_TEXT("\t\t\t\t\t(sequential if not set)\n")
    ACE_TEXT(" --default-try-construct\t\tset default XTypes try-construct behavior to ")
    ACE_TEXT("discard, use-default, or trim\n\t\t\t\t\t(discard if not set)\n")
    ACE_TEXT(" --old-typeobject-encoding\t\tuse the pre-3.18 encoding of TypeObjects when deriving TypeIdentifiers\n")
    ACE_TEXT(" -Wb,export_macro=<macro name>\t\tset export macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT("\t\t\t\t\t\t--export=<macro name> is an alternative form for this option\n")
    ACE_TEXT(" -Wb,export_include=<include path>\tset export include ")
    ACE_TEXT("file for all files\n")
    ACE_TEXT(" -Wb,versioning_name=<macro name>\tset versioning name macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_begin=<macro name>\tset versioning begin macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_end=<macro name>\tset versioning end macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,pch_include=<include path>\t\tset include ")
    ACE_TEXT("file for precompiled header mechanism\n")
    ACE_TEXT(" -Wb,cpp_include=<include path>\t\tset additional include ")
    ACE_TEXT("file for cpp files. Useful for 'after the fact'\n\t\t\t\t\ttypesupport generation ")
    ACE_TEXT("with different features enabled than in the\n\t\t\t\t\tfirst pass.\n")
    ACE_TEXT(" -Wb,java[=<output_file>]\t\tenable Java support ")
    ACE_TEXT("for TypeSupport files.  Do not specify an\n\t\t\t\t\t'output_file'")
    ACE_TEXT("except for special cases.\n")
    ACE_TEXT(" -Wb,tao_include_prefix=<path>\t\tprefix for including the TAO-")
    ACE_TEXT("generated header file.\n")
    ACE_TEXT(" -Wb,ts_cpp_include=<include>\t\tadd <include> to *TypeSupportImpl.cpp\n")
    ACE_TEXT(" -Wb,opendds_sequence_suffix=<suffix>\tset the implied DDS sequence suffix ")
    ACE_TEXT("(default is 'Seq')\n")
    ));
}

void BE_BuiltinInterface::rm_arg(int& i, int& argc, ACE_TCHAR* argv[])
{
  // Shift everything down one (including the NULL)
  const int down = 1;
  for (int j = i + 1; j <= argc; j++) {
    argv[j - down] = argv[j];
  }
  argc -= down;

  // Set i back to where it was when it was on the current option
  // after the next increment in the for loop.
  i -= down;
}

BE_BuiltinInterface::language_mapping_allocator
BE_BuiltinInterface::load_language_mapping(const ACE_TCHAR* mapping_name)
{
  language_mapping_allocator symbol = 0;
  if (mapping_name != 0) {
    ACE_TString dllname("opendds_idl_");
    dllname += mapping_name;
    ACE_TString symbolname(dllname);
    symbolname += ACE_TEXT("_allocator");
    ACE_DLL_Manager* manager = ACE_DLL_Manager::instance();
    if (manager != 0) {
      ACE_DLL_Handle* handle = manager->open_dll(dllname.c_str(), RTLD_NOW,
                                                 ACE_SHLIB_INVALID_HANDLE);
      if (handle != 0) {
        symbol = static_cast<language_mapping_allocator>(handle->symbol(symbolname.c_str()));
        if (symbol == 0) {
          manager->close_dll(dllname.c_str());
        }
      }
    }
  }
  return symbol;
}

void BE_BuiltinInterface::allocate_language_mapping(int& argc, ACE_TCHAR* argv[])
{
  const ACE_TCHAR* generic_mapping_opt = ACE_TEXT("-L");
  const size_t gmo_len = ACE_OS::strlen(generic_mapping_opt);

  for (int i = 1; i < argc; i++) {
    if (ACE_OS::strcasecmp(argv[i], ACE_TEXT("-Lspcpp")) == 0) {
      SPLanguageMapping* language_mapping = 0;
      ACE_NEW(language_mapping, SPLanguageMapping);
      be_builtin_global->language_mapping(language_mapping);
      rm_arg(i, argc, argv);
    }
    else if (ACE_OS::strcasecmp(argv[i], ACE_TEXT("-Lc++11")) == 0) {
      Cxx11LanguageMapping* language_mapping = 0;
      ACE_NEW(language_mapping, Cxx11LanguageMapping);
      be_builtin_global->language_mapping(language_mapping);
      rm_arg(i, argc, argv);
    }
    else if (ACE_OS::strncasecmp(argv[i], generic_mapping_opt, gmo_len) == 0) {
      language_mapping_allocator mapping = load_language_mapping(argv[i] + gmo_len);
      if (mapping != 0) {
        LanguageMapping* language_mapping = mapping();
        if (language_mapping != 0) {
          be_builtin_global->language_mapping(language_mapping);
          rm_arg(i, argc, argv);
        }
      }
    }
  }
}
