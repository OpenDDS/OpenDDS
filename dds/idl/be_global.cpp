/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_global.h"

#include "be_util.h"
#include "be_extern.h"

#include <ast_generator.h>
#include <global_extern.h>
#include <idl_defines.h>
#include <utl_err.h>
#include <utl_string.h>
#include <ast_decl.h>
#include <ast_structure.h>
#include <ast_field.h>
#include <ast_union.h>
#include <ast_annotation_decl.h>
#include <ast_annotation_member.h>

#include <ace/OS_NS_strings.h>
#include <ace/OS_NS_sys_stat.h>
#include <ace/ARGV.h>
#include <ace/OS_NS_stdlib.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

using namespace std;

BE_GlobalData* be_global = 0;

BE_GlobalData::BE_GlobalData()
  : filename_(0)
  , java_(false)
  , suppress_idl_(false)
  , suppress_typecode_(false)
  , no_default_gen_(false)
  , generate_itl_(false)
  , generate_v8_(false)
  , generate_rapidjson_(false)
  , face_ts_(false)
  , seq_("Seq")
  , language_mapping_(LANGMAP_NONE)
  , root_default_nested_(true)
  , warn_about_dcps_data_type_(true)
{
}

BE_GlobalData::~BE_GlobalData()
{
}

void
BE_GlobalData::destroy()
{
}

const char*
BE_GlobalData::filename() const
{
  return this->filename_;
}

void
BE_GlobalData::filename(const char* fname)
{
  this->filename_ = fname;
}

ACE_CString BE_GlobalData::export_macro() const
{
  return this->export_macro_;
}

void BE_GlobalData::export_macro(const ACE_CString& str)
{
  this->export_macro_ = str;
}

ACE_CString BE_GlobalData::export_include() const
{
  return this->export_include_;
}

void BE_GlobalData::export_include(const ACE_CString& str)
{
  this->export_include_ = str;
}

ACE_CString BE_GlobalData::versioning_name() const
{
  return this->versioning_name_;
}

void BE_GlobalData::versioning_name(const ACE_CString& str)
{
  this->versioning_name_ = str;
}

ACE_CString BE_GlobalData::versioning_begin() const
{
  return this->versioning_begin_;
}

void BE_GlobalData::versioning_begin(const ACE_CString& str)
{
  this->versioning_begin_ = str;
}

ACE_CString BE_GlobalData::versioning_end() const
{
  return this->versioning_end_;
}

void BE_GlobalData::versioning_end(const ACE_CString& str)
{
  this->versioning_end_ = str;
}

void BE_GlobalData::pch_include(const ACE_CString& str)
{
  this->pch_include_ = str;
}

ACE_CString BE_GlobalData::pch_include() const
{
  return this->pch_include_;
}

void BE_GlobalData::add_cpp_include(const std::string& str)
{
  this->cpp_includes_.insert(str);
}

const std::set<std::string>& BE_GlobalData::cpp_includes() const
{
  return this->cpp_includes_;
}

void BE_GlobalData::java_arg(const ACE_CString& str)
{
  this->java_arg_ = str;
}

ACE_CString BE_GlobalData::java_arg() const
{
  return this->java_arg_;
}

void BE_GlobalData::language_mapping(LanguageMapping lm)
{
  this->language_mapping_ = lm;
}

BE_GlobalData::LanguageMapping BE_GlobalData::language_mapping() const
{
  return this->language_mapping_;
}

void BE_GlobalData::sequence_suffix(const ACE_CString& str)
{
  this->seq_ = str;
}

ACE_CString BE_GlobalData::sequence_suffix() const
{
  return this->seq_;
}

void BE_GlobalData::java(bool b)
{
  this->java_ = b;
}

bool BE_GlobalData::java() const
{
  return this->java_;
}

void BE_GlobalData::no_default_gen(bool b)
{
  this->no_default_gen_ = b;
}

bool BE_GlobalData::no_default_gen() const
{
  return this->no_default_gen_;
}

void BE_GlobalData::itl(bool b)
{
  this->generate_itl_ = b;
}

bool BE_GlobalData::itl() const
{
  return this->generate_itl_;
}

void BE_GlobalData::v8(bool b)
{
  this->generate_v8_ = b;
}

bool BE_GlobalData::v8() const
{
  return this->generate_v8_;
}

void BE_GlobalData::rapidjson(bool b)
{
  this->generate_rapidjson_ = b;
}

bool BE_GlobalData::rapidjson() const
{
  return this->generate_rapidjson_;
}

void BE_GlobalData::face_ts(bool b)
{
  this->face_ts_ = b;
}

bool BE_GlobalData::face_ts() const
{
  return this->face_ts_;
}

void
BE_GlobalData::open_streams(const char* filename)
{
  size_t len = strlen(filename);
  if ((len < 5 || 0 != ACE_OS::strcasecmp(filename + len - 4, ".idl"))
      && (len < 6 || 0 != ACE_OS::strcasecmp(filename + len - 5, ".pidl"))) {
    ACE_ERROR((LM_ERROR, "Error - Input filename must end in \".idl\" or \".pidl\".\n"));
    BE_abort();
  }

  string filebase(filename);
  filebase.erase(filebase.rfind('.'));
  size_t idx = filebase.find_last_of("/\\"); // allow either slash
  if (idx != string::npos) {
    filebase = filebase.substr(idx + 1);
  }
  header_name_ = (filebase + "TypeSupportImpl.h").c_str();
  impl_name_ = (filebase + "TypeSupportImpl.cpp").c_str();
  idl_name_ = (filebase + "TypeSupport.idl").c_str();
  itl_name_ = (filebase + ".itl").c_str();
  facets_header_name_ = (filebase + "_TS.hpp").c_str();
  facets_impl_name_ = (filebase + "_TS.cpp").c_str();
  lang_header_name_ = (filebase + "C.h").c_str();
}

void
BE_GlobalData::multicast(const char* str)
{
  header_ << str;
  impl_ << str;
  idl_ << str;
  if (language_mapping_ != LANGMAP_NONE) lang_header_ << str;
}

BE_Comment_Guard::BE_Comment_Guard(const char* type, const char* name)
  : type_(type), name_(name)
{
  if (idl_global->compile_flags() & IDL_CF_INFORMATIVE)
    std::cout << type << ": " << name << std::endl;

  be_global->multicast("\n\n/* Begin ");
  be_global->multicast(type);
  be_global->multicast(": ");
  be_global->multicast(name);
  be_global->multicast(" */\n\n");
}

BE_Comment_Guard::~BE_Comment_Guard()
{
  be_global->multicast("\n/* End ");
  be_global->multicast(type_);
  be_global->multicast(": ");
  be_global->multicast(name_);
  be_global->multicast(" */\n");
}

ACE_CString
BE_GlobalData::spawn_options()
{
  return idl_global->idl_flags();
}

void invalid_option(char * option)
{
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("IDL: I don't understand the '%C' option\n"), option));
#ifdef TAO_IDL_HAS_PARSE_ARGS_EXIT
  idl_global->parse_args_exit(1);
#else
  idl_global->set_compile_flags(
    idl_global->compile_flags() | IDL_CF_ONLY_USAGE);
#endif
}

void
BE_GlobalData::parse_args(long& i, char** av)
{
  // This flag is provided for CIAO compatibility
  static const char EXPORT_FLAG[] = "--export=";
  static const size_t EXPORT_FLAG_SIZE = sizeof(EXPORT_FLAG) - 1;

  static const char DEFAULT_NESTED_FLAG[] = "--default-nested";
  static const size_t DEFAULT_NESTED_FLAG_SIZE = sizeof(DEFAULT_NESTED_FLAG) - 1;

  static const char NO_DEFAULT_NESTED_FLAG[] = "--no-default-nested";
  static const size_t NO_DEFAULT_NESTED_FLAG_SIZE = sizeof(NO_DEFAULT_NESTED_FLAG) - 1;

  static const char NO_DCPS_DATA_TYPE_WARNINGS_FLAG[] = "--no-dcps-data-type-warnings";
  static const size_t NO_DCPS_DATA_TYPE_WARNINGS_FLAG_SIZE = sizeof(NO_DCPS_DATA_TYPE_WARNINGS_FLAG) - 1;

  switch (av[i][1]) {
  case 'o':
    idl_global->append_idl_flag(av[i + 1]);
    if (ACE_OS::mkdir(av[i + 1]) != 0 && errno != EEXIST) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("IDL: unable to create directory %C")
        ACE_TEXT(" specified by -o option\n"), av[i + 1]));
#ifdef TAO_IDL_HAS_PARSE_ARGS_EXIT
      idl_global->parse_args_exit(1);
#endif
    } else {
      output_dir_ = av[++i];
    }
    break;

  case 'G':
    if (0 == ACE_OS::strcmp(av[i], "-Gitl")) {
      itl(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-GfaceTS")) {
      face_ts(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-Gv8")) {
      be_global->v8(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-Grapidjson")) {
      be_global->rapidjson(true);
    } else {
      invalid_option(av[i]);
    }
    break;

  case 'L':
    if (0 == ACE_OS::strcasecmp(av[i], "-Lface"))
      language_mapping(LANGMAP_FACE_CXX);
    else if (0 == ACE_OS::strcasecmp(av[i], "-Lspcpp"))
      language_mapping(LANGMAP_SP_CXX);
    else if (0 == ACE_OS::strcasecmp(av[i], "-Lc++11")) {
      language_mapping(LANGMAP_CXX11);
      suppress_typecode_ = true;
    } else {
      invalid_option(av[i]);
    }
    break;

  case 'S':
    if (0 == ACE_OS::strcasecmp(av[i], "-Sdefault")) {
      no_default_gen_ = true;
      break;
    }
    if (av[i][2] && av[i][3]) {
      invalid_option(av[i]);
      break;
    }
    switch (av[i][2]) {
    case 'I':
      suppress_idl_ = true;
      break;
    case 't':
      suppress_typecode_ = true;
      break;
    case 'a':
      // ignore, accepted for tao_idl compatibility
      break;
    default:
      invalid_option(av[i]);
    }
    break;

  case '-':
    if (!ACE_OS::strncasecmp(av[i], EXPORT_FLAG, EXPORT_FLAG_SIZE)) {
      this->export_macro(av[i] + EXPORT_FLAG_SIZE);
    } else if (!ACE_OS::strncasecmp(av[i], DEFAULT_NESTED_FLAG, DEFAULT_NESTED_FLAG_SIZE)) {
      root_default_nested_ = true;
    } else if (!ACE_OS::strncasecmp(av[i], NO_DEFAULT_NESTED_FLAG, NO_DEFAULT_NESTED_FLAG_SIZE)) {
      root_default_nested_ = false;
    } else if (!ACE_OS::strncasecmp(av[i], NO_DCPS_DATA_TYPE_WARNINGS_FLAG, NO_DCPS_DATA_TYPE_WARNINGS_FLAG_SIZE)) {
      warn_about_dcps_data_type_ = false;
    } else {
      invalid_option(av[i]);
    }
    break;

  default:
    invalid_option(av[i]);
  }
}


bool
BE_GlobalData::writeFile(const char* fileName, const string& content)
{
  string file = (be_global->output_dir_ == "")
    ? fileName : (string(be_global->output_dir_.c_str()) + '/' + fileName);
  ofstream ofs(file.c_str());

  if (!ofs) {
    cerr << "ERROR - couldn't open " << file << " for writing.\n";
    return false;
  }

  ofs << content;
  return !!ofs;
}

//include file management (assumes a singleton BE_GlobalData object)

namespace {
  typedef set<string> Includes_t;
  Includes_t inc_h_, inc_c_, inc_idl_, referenced_idl_, inc_path_, inc_facets_h_,
    inc_lang_h_;
}

void
BE_GlobalData::reset_includes()
{
  inc_h_.clear();
  inc_c_.clear();
  inc_idl_.clear();
  inc_facets_h_.clear();
  inc_lang_h_.clear();
  referenced_idl_.clear();
}

void
BE_GlobalData::add_inc_path(const char* path)
{
  inc_path_.insert(path);
}

void
BE_GlobalData::set_inc_paths(const char* cmdline)
{
  ACE_ARGV argv(ACE_TEXT_CHAR_TO_TCHAR(cmdline), false);
  for (int i = 0; i < argv.argc(); ++i) {
    std::string arg = ACE_TEXT_ALWAYS_CHAR(argv[i]);
    if (arg == "-I" && i + 1 < argv.argc()) {
      inc_path_.insert(ACE_TEXT_ALWAYS_CHAR(argv[++i]));
    } else if (arg.substr(0, 2) == "-I") {
      inc_path_.insert(arg.c_str() + 2);
    }
  }
}

void
BE_GlobalData::add_include(const char* file,
                           BE_GlobalData::stream_enum_t which)
{
  Includes_t* inc = 0;

  switch (which) {
  case STREAM_H:
    inc = &inc_h_;
    break;
  case STREAM_CPP:
    inc = &inc_c_;
    break;
  case STREAM_IDL:
    inc = &inc_idl_;
    break;
  case STREAM_FACETS_H:
    inc = &inc_facets_h_;
    break;
  case STREAM_LANG_H:
    inc = &inc_lang_h_;
    break;
  default:
    return;
  }

  inc->insert(file);
}

void
BE_GlobalData::add_referenced(const char* file)
{
  referenced_idl_.insert(file);
}

namespace {
  std::string transform_referenced(const std::string& idl, const char* suffix)
  {
    const size_t len = idl.size();
    string base_name;
    if (len >= 5 && 0 == ACE_OS::strcasecmp(idl.c_str() + len - 4, ".idl")) {
      base_name.assign(idl.c_str(), len - 4);

    } else if (len >= 6 &&
        0 == ACE_OS::strcasecmp(idl.c_str() + len - 5, ".pidl")) {
      base_name.assign(idl.c_str(), len - 5);
      size_t slash = base_name.find_last_of("/\\");
      if (slash != std::string::npos && slash > 3 && base_name.size() > 3
          && base_name.substr(slash - 3, 3) == "tao"
          && base_name.substr(base_name.size() - 3) == "Seq") {
        base_name = "dds/CorbaSeq/" + base_name.substr(slash + 1);
      }
    }

    return base_name + suffix;
  }

  std::string make_relative(const std::string& absolute)
  {
    for (Includes_t::const_iterator iter = inc_path_.begin(),
        end = inc_path_.upper_bound(absolute); iter != end; ++iter) {
      if (absolute.find(*iter) == 0) {
        string rel = absolute.substr(iter->size());
        if (rel.size() && (rel[0] == '/' || rel[0] == '\\')) {
          rel.erase(0, 1);
        }
        return rel;
      }
    }
    return absolute;
  }

  struct InsertIncludes {
    std::ostream& ret_;
    explicit InsertIncludes(std::ostream& ret) : ret_(ret) {}

    void operator()(const std::string& str) const
    {
       const char* const quote = (!str.empty() && str[0] != '<') ? "\"" : "";
       ret_ << "#include " << quote << str << quote << '\n';
    }
  };

  struct InsertRefIncludes : InsertIncludes {
    const char* const suffix_;

    InsertRefIncludes(std::ostream& ret, const char* suffix)
      : InsertIncludes(ret)
      , suffix_(suffix)
    {}

    void operator()(const std::string& str) const
    {
      InsertIncludes::operator()(transform_referenced(make_relative(str), suffix_));
    }
  };
}

std::string
BE_GlobalData::get_include_block(BE_GlobalData::stream_enum_t which)
{
  const Includes_t* inc = 0;

  switch (which) {
  case STREAM_H:
    inc = &inc_h_;
    break;
  case STREAM_CPP:
    inc = &inc_c_;
    break;
  case STREAM_IDL:
    inc = &inc_idl_;
    break;
  case STREAM_FACETS_H:
    inc = &inc_facets_h_;
    break;
  case STREAM_LANG_H:
    inc = &inc_lang_h_;
    break;
  default:
    return "";
  }

  std::ostringstream ret;

  std::for_each(inc->begin(), inc->end(), InsertIncludes(ret));

  switch (which) {
  case STREAM_LANG_H:
    std::for_each(referenced_idl_.begin(), referenced_idl_.end(),
                  InsertRefIncludes(ret, "C.h"));
    // fall through
  case STREAM_H:
    if (!export_include().empty())
      ret << "#include \"" << export_include() << "\"\n";
    break;
  case STREAM_CPP:
    std::for_each(cpp_includes().begin(), cpp_includes().end(), InsertIncludes(ret));
    std::for_each(referenced_idl_.begin(), referenced_idl_.end(),
                  InsertRefIncludes(ret, "TypeSupportImpl.h"));
    break;
  default:
    break;
  }

  return ret.str();
}

bool BE_GlobalData::is_topic_type(AST_Decl* node)
{
  return builtin_annotations_["::@topic"]->find_on(node) || !is_nested(node);
}

bool BE_GlobalData::is_nested(AST_Decl* node)
{
  NestedAnnotation* nested = dynamic_cast<NestedAnnotation*>(
    builtin_annotations_["::@nested"]);
  if (nested->find_on(node)) {
    return nested->node_value(node);
  }

  return is_default_nested(node->defined_in());
}

bool BE_GlobalData::is_default_nested(UTL_Scope* scope)
{
  AST_Decl* module = dynamic_cast<AST_Decl*>(scope);
  DefaultNestedAnnotation* default_nested = dynamic_cast<DefaultNestedAnnotation*>(
    builtin_annotations_["::@default_nested"]);
  if (module) {
    if (default_nested->find_on(module)) {
      return default_nested->node_value(module);
    }

    return is_default_nested(module->defined_in());
  }

  return root_default_nested_;
}

bool BE_GlobalData::check_key(AST_Field* node, bool& value)
{
  KeyAnnotation* key = dynamic_cast<KeyAnnotation*>(builtin_annotations_["::@key"]);
  return key->node_value_exists(node, value);
}

bool BE_GlobalData::has_key(AST_Union* node)
{
  KeyAnnotation* key = dynamic_cast<KeyAnnotation*>(builtin_annotations_["::@key"]);
  return key->union_value(node);
}

void BE_GlobalData::warning(const char* msg, const char* filename, unsigned lineno)
{
  if (idl_global->print_warnings()) {
    if (filename) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("Warning - %C: \"%C\", line %u: %C\n"),
        idl_global->prog_name(), filename, lineno, msg));
    } else {
      ACE_ERROR((LM_WARNING, ACE_TEXT("Warning - %C: %C\n"),
        idl_global->prog_name(), msg));
    }
  }
  idl_global->err()->last_warning = UTL_Error::EIDL_MISC;
}

void BE_GlobalData::error(const char* msg, const char* filename, unsigned lineno)
{
  if (filename) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error - %C: \"%C\", line %u: %C\n"),
      idl_global->prog_name(), filename, lineno, msg));
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error - %C: %C\n"),
      idl_global->prog_name(), msg));
  }
  idl_global->set_err_count(idl_global->err_count() + 1);
  idl_global->err()->last_error = UTL_Error::EIDL_MISC;
}

bool BE_GlobalData::warn_about_dcps_data_type()
{
  if (!warn_about_dcps_data_type_) {
    return false;
  }
  warn_about_dcps_data_type_ = false;
  return idl_global->print_warnings();
}
