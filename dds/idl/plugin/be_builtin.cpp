/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_builtin.h"
#include "be_util.h"
#include "dds/Version.h"

#include "cxx11_language_mapping.h"
#include "sp_language_mapping.h"

#include <global_extern.h>
#include <drv_extern.h>

#include <ace/DLL_Manager.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_strings.h>

#include <iostream>
#include <iomanip>

BE_BuiltinInterface* BE_BuiltinInterface::instance()
{
  static BE_BuiltinInterface instance_;
  return &instance_;
}

BE_BuiltinInterface::BE_BuiltinInterface()
{
}

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

// Do the work of this BE. This is the starting point for code generation.
void
BE_BuiltinInterface::produce()
{
  be_builtin_global->language_mapping()->produce();
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
    ACE_TEXT(" -Lspcpp\t\tgenerate Safety Profile IDL to C++ mapping\n")
    ACE_TEXT(" -Lc++11\t\tgenerate IDL to C++11 mapping\n")
    ACE_TEXT(" -L<libname>\t\tdynamically load a language mapping plugin\n")
    ACE_TEXT(" -G<libname>TS\t\tgenerate TS API for DCPS data types when used with -L\n")
    ACE_TEXT(" -SI\t\t\tsuppress generation of *TypeSupport.idl\n")
    ACE_TEXT(" -Sa\t\t\tsuppress IDL any (ignored, for tao_idl compatibility)\n")
    ACE_TEXT(" -St\t\t\tsuppress IDL typecode when -L* option is present\n")
    ACE_TEXT(" -Sv\t\t\tsuppress ValueReader and ValueWriter generation\n")
    ACE_TEXT(" -Sx\t\t\tsuppress XTypes TypeObject and TypeIdentifier\n\t\t\tgeneration\n")
    ACE_TEXT(" -Sdefault\t\texclude default TypeSupport generators from output\n")
    ACE_TEXT(" -Gitl\t\t\tgenerate ITL\n")
    ACE_TEXT(" -Grapidjson\t\tgenerate TypeSupport for converting data samples\n")
    ACE_TEXT("\t\t\tto RapidJSON JavaScript objects\n")
    ACE_TEXT(" -Gxtypes-complete\tgenerate XTypes complete TypeObject and TypeIdentifier\n")
    ACE_TEXT(" --filename-only-includes\tStrip leading directories from generated\n\t\t\t\t#include lines\n")
    ACE_TEXT(" --[no-]default-nested\t\ttopic types must be declared, true by default\n")
    ACE_TEXT(" --no-dcps-data-type-warnings\tdon't warn about #pragma DCPS_DATA_TYPE\n")
    ACE_TEXT(" --default-extensibility\tset default XTypes extensibility kind to final,\n")
    ACE_TEXT("\t\t\t\tappendable, or mutable (appendable if not set)\n")
    ACE_TEXT(" --default-autoid\t\tset default XTypes autoid approach to\n")
    ACE_TEXT("\t\t\t\tsequential or hash (sequential if not set)\n")
    ACE_TEXT(" --default-try-construct\tset default XTypes try-construct behavior to\n")
    ACE_TEXT("\t\t\t\tdiscard, use-default, or trim\n\t\t\t\t(discard if not set)\n")
    ACE_TEXT(" --old-typeobject-encoding\tuse the pre-3.18 encoding of TypeObjects when\n\t\t\t\tderiving TypeIdentifiers\n")
    ACE_TEXT(" -Wb,export_macro=<macro>\tset export macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT("\t\t\t\t--export=<macro> is an alternative form\n")
    ACE_TEXT(" -Wb,export_include=<path>\tset export include ")
    ACE_TEXT("file for all files\n")
    ACE_TEXT(" -Wb,versioning_name=<macro>\tset versioning name macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_begin=<macro>\tset versioning begin macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,versioning_end=<macro>\tset versioning end macro ")
    ACE_TEXT("for all files\n")
    ACE_TEXT(" -Wb,pch_include=<file>\t\tset include ")
    ACE_TEXT("file for precompiled headers\n")
    ACE_TEXT(" -Wb,cpp_include=<file>\t\tset additional include ")
    ACE_TEXT("file for cpp files.\n\t\t\t\tUseful for 'after the fact' typesupport\n")
    ACE_TEXT("\t\t\t\tgeneration with different features enabled\n")
    ACE_TEXT("\t\t\t\tthan in the first pass.\n")
    ACE_TEXT(" -Wb,java[=<output_file>]\tenable Java support for TypeSupport files.  Do\n")
    ACE_TEXT("\t\t\t\tnot specify an 'output_file' except for special\n")
    ACE_TEXT("\t\t\t\tcases.\n")
    ACE_TEXT(" -Wb,tao_include_prefix=<path>\tprefix for including the TAO-generated header\n")
    ACE_TEXT("\t\t\t\tfile.\n")
    ACE_TEXT(" -Wb,ts_cpp_include=<include>\tadd <include> to *TypeSupportImpl.cpp\n")
    ACE_TEXT(" -Wb,opendds_sequence_suffix=<suffix>\tset the implied DDS sequence suffix\n")
    ACE_TEXT("\t\t\t\t\t(default is 'Seq')\n")
    ));

  be_builtin_global->language_mapping()->usage();
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
BE_BuiltinInterface::load_language_mapping(const ACE_TString& mapping_name)
{
  language_mapping_allocator symbol = 0;
  ACE_TString dllname("opendds_idl_");
  dllname += mapping_name;
  ACE_TString symbolname(dllname);
  symbolname += ACE_TEXT("_allocator");
  ACE_DLL_Manager* manager = ACE_DLL_Manager::instance();
  if (manager != 0) {
    ACE_DLL_Handle* handle = manager->open_dll(dllname.c_str(), RTLD_NOW,
                                                ACE_SHLIB_INVALID_HANDLE);
    if (handle == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("Unable to load language mapping: %s\n"), dllname.c_str()));
    }
    else {
      symbol = reinterpret_cast<language_mapping_allocator>(handle->symbol(symbolname.c_str()));
      if (symbol == 0) {
        manager->close_dll(dllname.c_str());
      }
      else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("Unable to find the language mapping symbol: %s\n"),
                   symbolname.c_str()));
      }
    }
  }
  return symbol;
}

void BE_BuiltinInterface::allocate_language_mapping(int& argc, ACE_TCHAR* argv[])
{
  // Set up for processing the generic -L and -G options.
  const ACE_TString generic_mapping_opt(ACE_TEXT("-L"));
  const ACE_TString generic_ts_opt(ACE_TEXT("-G"));

  // We need to keep track of which dynamically loaded language mappings the
  // user has requested and the possibility of an associated -G option.
  typedef std::pair<LanguageMapping*, bool> TSMapValue;
  typedef std::map<ACE_TString, TSMapValue> TSMap;
  TSMap ts_mapping;

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
    else if (ACE_OS::strncasecmp(argv[i], generic_mapping_opt.c_str(),
                                 generic_mapping_opt.length()) == 0) {
      // Attempt to load the language mapping based on the portion of the
      // option after the -L.
      const ACE_TString key(argv[i] + generic_mapping_opt.length());
      language_mapping_allocator mapping = load_language_mapping(key);
      if (mapping != 0) {
        LanguageMapping* language_mapping = mapping();
        if (language_mapping != 0) {
          // Add or update the map to contain the language mapping pointer.
          TSMap::iterator found = ts_mapping.find(key);
          if (found == ts_mapping.end()) {
            ts_mapping.insert(TSMap::value_type(key, TSMapValue(language_mapping, false)));
          }
          else {
            found->second.first = language_mapping;
          }
          be_builtin_global->language_mapping(language_mapping);
          rm_arg(i, argc, argv);
        }
      }
    }
    else if (ACE_OS::strncasecmp(argv[i], generic_ts_opt.c_str(), generic_ts_opt.length()) == 0) {
      // See if the portion after the -G option ends in "TS".
      const ACE_TString suffix(ACE_TEXT("TS"));
      const size_t arglen = ACE_OS::strlen(argv[i]);
      if (arglen > suffix.length() &&
          ACE_OS::strcasecmp(argv[i] + arglen - suffix.length(), suffix.c_str()) == 0) {
        // If it does, create a key based on the portion in between the -G and
        // TS.  Due to a limitation of ACE_TString, we have to reassign the key
        // to itself after chopping off the TS portion.
        ACE_TString key(argv[i] + generic_ts_opt.length());
        size_t klen = key.length();
        if (klen > suffix.length()) {
          key[klen - suffix.length()] = '\0';
          key = ACE_TString(key.c_str());

          // Add or update the boolean for the TS option.
          TSMap::iterator found = ts_mapping.find(key);
          if (found == ts_mapping.end()) {
            ts_mapping.insert(TSMap::value_type(key, TSMapValue(0, true)));
          }
          else {
            found->second.second = true;
          }
          rm_arg(i, argc, argv);
        }
      }
    }
  }

  // Iterate over the TS mapping and see if any language mappings require us
  // to set the TS option.  We used a map because the order of the -L and -G
  // options is completely up to the user and the -G option could come before
  // the -L option (which wouldn't be handled if we didn't).
  TSMap::iterator end = ts_mapping.end();
  for (TSMap::iterator itr = ts_mapping.begin();
       itr != end; ++itr) {
    if (itr->second.first != 0 && itr->second.second) {
      itr->second.first->setTS(true);
    }
  }
}
