/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_global.h"

#include "be_util.h"
#include "be_extern.h"
#include "dds_generator.h"

#include "dds/DCPS/XTypes/TypeObject.h"

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
#include <cstring>
#include <vector>
#include <set>

using namespace std;

BE_GlobalData* be_global = 0;

BE_GlobalData::BE_GlobalData()
  : filename_(0)
  , java_(false)
  , suppress_idl_(false)
  , suppress_typecode_(false)
  , suppress_xtypes_(false)
  , no_default_gen_(false)
  , generate_itl_(false)
  , generate_value_reader_writer_(true)
  , generate_xtypes_complete_(false)
  , face_ts_(false)
  , generate_equality_(false)
  , filename_only_includes_(false)
  , sequence_suffix_("Seq")
  , language_mapping_(LANGMAP_NONE)
  , root_default_nested_(true)
  , warn_about_dcps_data_type_(true)
  , default_extensibility_(extensibilitykind_appendable)
  , default_enum_extensibility_zero_(false)
  , root_default_autoid_(autoidkind_sequential)
  , default_try_construct_(tryconstructfailaction_discard)
  , old_typeobject_encoding_(false)
  , old_typeobject_member_order_(false)
{
  default_data_representation_.set_all(true);

  platforms_.insert("*");
  platforms_.insert("DDS");
  platforms_.insert("OpenDDS");
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

void BE_GlobalData::add_cpp_include(const string& str)
{
  this->cpp_includes_.insert(make_pair(str, ""));
}

const set<pair<string, string> >& BE_GlobalData::cpp_includes() const
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
  this->sequence_suffix_ = str;
}

ACE_CString BE_GlobalData::sequence_suffix() const
{
  return this->sequence_suffix_;
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

void BE_GlobalData::filename_only_includes(bool b)
{
  this->filename_only_includes_ = b;
}

bool BE_GlobalData::filename_only_includes() const
{
  return this->filename_only_includes_;
}


void BE_GlobalData::itl(bool b)
{
  this->generate_itl_ = b;
}

bool BE_GlobalData::itl() const
{
  return this->generate_itl_;
}

void BE_GlobalData::value_reader_writer(bool b)
{
  this->generate_value_reader_writer_ = b;
}

bool BE_GlobalData::value_reader_writer() const
{
  return this->generate_value_reader_writer_;
}

void BE_GlobalData::face_ts(bool b)
{
  this->face_ts_ = b;
}

bool BE_GlobalData::face_ts() const
{
  return this->face_ts_;
}

void BE_GlobalData::xtypes_complete(bool b)
{
  this->generate_xtypes_complete_ = b;
}

bool BE_GlobalData::xtypes_complete() const
{
  return this->generate_xtypes_complete_;
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
    cout << type << ": " << name << endl;

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

void invalid_option(char* option)
{
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("opendds_idl: I don't understand the '%C' option\n"), option));
  idl_global->parse_args_exit(1);
}

void
BE_GlobalData::parse_args(long& i, char** av)
{
  static const char EXPORT_FLAG[] = "--export=";
  static const size_t EXPORT_FLAG_SIZE = sizeof(EXPORT_FLAG) - 1;

  static const char DEFAULT_NESTED_FLAG[] = "--default-nested";
  static const size_t DEFAULT_NESTED_FLAG_SIZE = sizeof(DEFAULT_NESTED_FLAG) - 1;

  static const char NO_DEFAULT_NESTED_FLAG[] = "--no-default-nested";
  static const size_t NO_DEFAULT_NESTED_FLAG_SIZE = sizeof(NO_DEFAULT_NESTED_FLAG) - 1;

  static const char NO_DCPS_DATA_TYPE_WARNINGS_FLAG[] = "--no-dcps-data-type-warnings";
  static const size_t NO_DCPS_DATA_TYPE_WARNINGS_FLAG_SIZE = sizeof(NO_DCPS_DATA_TYPE_WARNINGS_FLAG) - 1;

  static const char FILENAME_ONLY_INCLUDES_FLAG[] = "--filename-only-includes";
  static const size_t FILENAME_ONLY_INCLUDES_FLAG_SIZE = sizeof(FILENAME_ONLY_INCLUDES_FLAG) - 1;

  switch (av[i][1]) {
  case 'o':
    if (av[++i] == 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("No argument for -o\n")));
      idl_global->parse_args_exit(1);
    } else {
      idl_global->append_idl_flag(av[i]);
      if (ACE_OS::mkdir(av[i]) != 0 && errno != EEXIST) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: unable to create directory %C")
          ACE_TEXT(" specified by -o option\n"), av[i]));
        idl_global->parse_args_exit(1);
      } else {
        output_dir_ = av[i];
      }
    }
    break;

  case 'G':
    if (0 == ACE_OS::strcmp(av[i], "-Gitl")) {
      itl(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-GfaceTS")) {
      face_ts(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-Gxtypes-complete")) {
      xtypes_complete(true);
    } else if (0 == ACE_OS::strcasecmp(av[i], "-Gequality")) {
      generate_equality(true);
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
    case 'v':
      generate_value_reader_writer_ = false;
      break;
    case 'x':
      suppress_xtypes_ = true;
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
    } else if (!ACE_OS::strncasecmp(av[i], FILENAME_ONLY_INCLUDES_FLAG, FILENAME_ONLY_INCLUDES_FLAG_SIZE)) {
      filename_only_includes_ = true;
    } else if (!strcmp(av[i], "--default-extensibility")) {
      if (av[++i] == 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("No argument for --default-extensibility\n")));
        idl_global->parse_args_exit(1);
      } else if (!strcmp(av[i], "final")) {
        default_extensibility_ = extensibilitykind_final;
      } else if (!strcmp(av[i], "appendable")) {
        default_extensibility_ = extensibilitykind_appendable;
      } else if (!strcmp(av[i], "mutable")) {
        default_extensibility_ = extensibilitykind_mutable;
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("Invalid argument to --default-extensibility: %C\n"), av[i]));
        idl_global->parse_args_exit(1);
      }
    } else if (!strcmp(av[i], "--default-enum-extensibility-zero")) {
      default_enum_extensibility_zero_ = true;
    } else if (!strcmp(av[i], "--default-autoid")) {
      if (av[++i] == 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("No argument for --default-autoid\n")));
        idl_global->parse_args_exit(1);
      } else if (!strcmp(av[i], "sequential")) {
        root_default_autoid_ = autoidkind_sequential;
      } else if (!strcmp(av[i], "hash")) {
        root_default_autoid_ = autoidkind_hash;
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("Invalid argument to --default-autoid: %C\n"), av[i]));
        idl_global->parse_args_exit(1);
      }
    } else if (!strcmp(av[i], "--default-try-construct")) {
      if (av[++i] == 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("No argument for --default-try-construct\n")));
        idl_global->parse_args_exit(1);
      } else if (!strcmp(av[i], "discard")) {
        default_try_construct_ = tryconstructfailaction_discard;
      } else if (!strcmp(av[i], "use-default")) {
        default_try_construct_ = tryconstructfailaction_use_default;
      } else if (!strcmp(av[i], "trim")) {
        default_try_construct_ = tryconstructfailaction_trim;
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("Invalid argument to --default-try-construct: %C\n"), av[i]));
        idl_global->parse_args_exit(1);
      }
    } else if (!strcmp(av[i], "--old-typeobject-encoding")) {
      old_typeobject_encoding_ = true;
    } else if (!strcmp(av[i], "--old-typeobject-member-order")) {
      old_typeobject_member_order_ = true;
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
  typedef set<pair<string, string> > Includes;
  Includes all_includes[BE_GlobalData::STREAM_COUNT];
  set<string> referenced_idl, inc_path;
  vector<string> inc_path_vector;
}

void
BE_GlobalData::reset_includes()
{
  inc_path_vector.clear();
  for (int i = 0; i < BE_GlobalData::STREAM_COUNT; ++i) {
    all_includes[i].clear();
  }
}

void
BE_GlobalData::add_inc_path(const char* path)
{
  if (inc_path.insert(path).second) {
    inc_path_vector.push_back(path);
  }
}

void
BE_GlobalData::set_inc_paths(const char* cmdline)
{
  ACE_ARGV argv(ACE_TEXT_CHAR_TO_TCHAR(cmdline), false);
  for (int i = 0; i < argv.argc(); ++i) {
    string arg = ACE_TEXT_ALWAYS_CHAR(argv[i]);
    if (arg == "-I" && i + 1 < argv.argc()) {
      add_inc_path(ACE_TEXT_ALWAYS_CHAR(argv[++i]));
    } else if (arg.substr(0, 2) == "-I") {
      add_inc_path(arg.c_str() + 2);
    }
  }
}

void
BE_GlobalData::add_include(const char* file, stream_enum_t which)
{
  conditional_include(file, which, "");
}

void
BE_GlobalData::conditional_include(const char* file,
                                   stream_enum_t which,
                                   const char* condition)
{
  all_includes[which].insert(make_pair(file, condition));
}

void
BE_GlobalData::add_referenced(const char* file)
{
  referenced_idl.insert(file);
}

namespace {
  pair<string, string> transform_referenced(const string& idl, const char* suffix)
  {
    const size_t len = idl.size();
    string base_name;
    if (len >= 5 && 0 == ACE_OS::strcasecmp(idl.c_str() + len - 4, ".idl")) {
      base_name.assign(idl.c_str(), len - 4);

    } else if (len >= 6 &&
        0 == ACE_OS::strcasecmp(idl.c_str() + len - 5, ".pidl")) {
      base_name.assign(idl.c_str(), len - 5);
      const size_t slash = base_name.find_last_of("/\\");
      if (slash != string::npos && slash >= 3 && base_name.size() > 3
          && base_name.substr(slash - 3, 3) == "tao"
          && base_name.substr(base_name.size() - 3) == "Seq") {
        base_name = "dds/CorbaSeq/" + base_name.substr(slash + 1);
      }
    }

    return make_pair(base_name + suffix, "");
  }

  string make_relative(const string& absolute, bool filename_only_includes)
  {
    for (vector<string>::reverse_iterator iter = inc_path_vector.rbegin(),
        end = inc_path_vector.rend(); iter != end; ++iter) {
      if (absolute.find(*iter) == 0) {
        string rel = absolute.substr(iter->size());

        if (rel.size() && (rel[0] == '/' || rel[0] == '\\')) {
          rel.erase(0, 1);
        }

        if (filename_only_includes) {
          size_t loc = rel.rfind('/', rel.length());
          const size_t locw = rel.rfind('\\', rel.length());

          if (loc != string::npos && locw != string::npos) {
            // path may contain both '/' and '\'. choose the last one.
            loc = loc > locw ? loc : locw;
          } else if (loc == string::npos) {
            loc = locw;
          }

          if (loc != string::npos) {
            rel = rel.substr(loc + 1, rel.length() - loc);
          }
        }

        return rel;
      }
    }

    return absolute;
  }

  struct InsertIncludes {
    ostream& ret_;
    explicit InsertIncludes(ostream& ret) : ret_(ret) {}

    void operator()(const pair<string, string>& inc) const
    {
      const string& str = inc.first;
      const char* const quote = (!str.empty() && str[0] != '<') ? "\"" : "";
      if (inc.second.size()) {
        ret_ << inc.second << "\n  ";
      }
      ret_ << "#include " << quote << str << quote << '\n';
      if (inc.second.size()) {
        ret_ << "#endif\n";
      }
    }
  };

  struct InsertRefIncludes : InsertIncludes {
    const char* const suffix_;
    bool filename_only_includes_;

    InsertRefIncludes(ostream& ret, const char* suffix, const bool filename_only_includes)
      : InsertIncludes(ret)
      , suffix_(suffix)
      , filename_only_includes_(filename_only_includes)
    {}

    void operator()(const string& str) const
    {
      InsertIncludes::operator()(transform_referenced(make_relative(str, filename_only_includes_), suffix_));
    }
  };
}

string
BE_GlobalData::get_include_block(BE_GlobalData::stream_enum_t which)
{
  Includes& inc = all_includes[which];
  ostringstream ret;

  for_each(inc.begin(), inc.end(), InsertIncludes(ret));

  switch (which) {
  case STREAM_LANG_H:
    for_each(referenced_idl.begin(), referenced_idl.end(),
             InsertRefIncludes(ret, "C.h", filename_only_includes_));
    // fall through
  case STREAM_H:
    if (!export_include().empty())
      ret << "#include \"" << export_include() << "\"\n";
    break;
  case STREAM_CPP:
    for_each(cpp_includes().begin(), cpp_includes().end(), InsertIncludes(ret));
    for_each(referenced_idl.begin(), referenced_idl.end(),
             InsertRefIncludes(ret, "TypeSupportImpl.h", filename_only_includes_));
    break;
  default:
    break;
  }

  return ret.str();
}

bool BE_GlobalData::is_topic_type(AST_Decl* node)
{
  return !is_nested(node);
}

bool BE_GlobalData::is_nested(AST_Decl* node)
{
  {
    // @topic overrides @nested and @default_nested.
    TopicAnnotation* topic = dynamic_cast<TopicAnnotation*>(builtin_annotations_["::@topic"]);
    TopicValue value;
    if (topic->node_value_exists(node, value)) {
      if (platforms_.count(value.platform)) {
        return false;
      }
    }
  }

  NestedAnnotation* nested = dynamic_cast<NestedAnnotation*>(
    builtin_annotations_["::@nested"]);
  bool value;
  if (nested->node_value_exists(node, value)) {
    return value;
  }

  return is_default_nested(node->defined_in());
}

bool BE_GlobalData::is_default_nested(UTL_Scope* scope)
{
  AST_Decl* module = dynamic_cast<AST_Decl*>(scope);
  DefaultNestedAnnotation* default_nested = dynamic_cast<DefaultNestedAnnotation*>(
    builtin_annotations_["::@default_nested"]);
  if (module) {
    bool value;
    if (default_nested->node_value_exists(module, value)) {
      return value;
    }

    return is_default_nested(module->defined_in());
  }

  return root_default_nested_;
}

bool BE_GlobalData::check_key(AST_Decl* node, bool& value) const
{
  KeyAnnotation* key = dynamic_cast<KeyAnnotation*>(builtin_annotations_["::@key"]);
  value = key->absent_value;
  return key->node_value_exists(node, value);
}

bool BE_GlobalData::union_discriminator_is_key(AST_Union* node)
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

ExtensibilityKind BE_GlobalData::extensibility(AST_Decl* node, ExtensibilityKind default_extensibility, bool& has_annotation) const
{
  has_annotation = true;

  if (builtin_annotations_["::@final"]->find_on(node)) {
    return extensibilitykind_final;
  }

  if (builtin_annotations_["::@appendable"]->find_on(node)) {
    return extensibilitykind_appendable;
  }

  if (builtin_annotations_["::@mutable"]->find_on(node)) {
    return extensibilitykind_mutable;
  }

  ExtensibilityAnnotation* extensibility_annotation =
    dynamic_cast<ExtensibilityAnnotation*>(
      builtin_annotations_["::@extensibility"]);
  ExtensibilityKind value;
  if (!extensibility_annotation->node_value_exists(node, value)) {
    value = default_extensibility;
    has_annotation = false;
  }
  return value;
}

ExtensibilityKind BE_GlobalData::extensibility(AST_Decl* node, ExtensibilityKind default_extensibility) const
{
  if (builtin_annotations_["::@final"]->find_on(node)) {
    return extensibilitykind_final;
  }

  if (builtin_annotations_["::@appendable"]->find_on(node)) {
    return extensibilitykind_appendable;
  }

  if (builtin_annotations_["::@mutable"]->find_on(node)) {
    return extensibilitykind_mutable;
  }

  ExtensibilityAnnotation* extensibility_annotation =
    dynamic_cast<ExtensibilityAnnotation*>(
      builtin_annotations_["::@extensibility"]);
  ExtensibilityKind value;
  if (!extensibility_annotation->node_value_exists(node, value)) {
    value = default_extensibility;
  }
  return value;
}

ExtensibilityKind BE_GlobalData::extensibility(AST_Decl* node) const
{
  return extensibility(node, default_extensibility_);
}

AutoidKind BE_GlobalData::autoid(AST_Decl* node) const
{
  AutoidAnnotation* autoid_annotation = dynamic_cast<AutoidAnnotation*>(
    builtin_annotations_["::@autoid"]);
  AutoidKind value;
  if (autoid_annotation->node_value_exists(node, value)) {
    return value;
  }
  return scoped_autoid(node->defined_in());
}

AutoidKind BE_GlobalData::scoped_autoid(UTL_Scope* scope) const
{
  AST_Decl* module = dynamic_cast<AST_Decl*>(scope);
  AutoidAnnotation* autoid_annotation = dynamic_cast<AutoidAnnotation*>(
    builtin_annotations_["::@autoid"]);
  if (module) {
    AutoidKind value;
    if (autoid_annotation->node_value_exists(module, value)) {
      return value;
    }
    return scoped_autoid(module->defined_in());
  }
  return root_default_autoid_;
}

bool BE_GlobalData::id(AST_Decl* node, ACE_CDR::ULong& value) const
{
  IdAnnotation* id_annotation =
    dynamic_cast<IdAnnotation*>(builtin_annotations_["::@id"]);
  return id_annotation->node_value_exists(node, value);
}

bool BE_GlobalData::hashid(AST_Decl* node, string& value) const
{
  HashidAnnotation* hashid_annotation =
    dynamic_cast<HashidAnnotation*>(builtin_annotations_["::@hashid"]);
  return hashid_annotation->node_value_exists(node, value);
}

bool BE_GlobalData::is_optional(AST_Decl* node) const
{
  OptionalAnnotation* optional_annotation =
    dynamic_cast<OptionalAnnotation*>(
      builtin_annotations_["::@optional"]);
  bool value = optional_annotation->absent_value;
  optional_annotation->node_value_exists(node, value);
  return value;
}

bool BE_GlobalData::is_must_understand(AST_Decl* node) const
{
  MustUnderstandAnnotation* must_understand_annotation =
    dynamic_cast<MustUnderstandAnnotation*>(
      builtin_annotations_["::@must_understand"]);
  bool value = must_understand_annotation->absent_value;
  must_understand_annotation->node_value_exists(node, value);
  return value;
}

bool BE_GlobalData::is_effectively_must_understand(AST_Decl* node) const
{
  return is_must_understand(node) || is_key(node);
}

bool BE_GlobalData::is_key(AST_Decl* node) const
{
  bool value;
  check_key(node, value);
  return value;
}

bool BE_GlobalData::is_external(AST_Decl* node) const
{
  ExternalAnnotation* external_annotation =
    dynamic_cast<ExternalAnnotation*>(
      builtin_annotations_["::@external"]);
  bool value = external_annotation->absent_value;
  external_annotation->node_value_exists(node, value);
  return value;
}

bool BE_GlobalData::is_plain(AST_Decl* node) const
{
  ExternalAnnotation* external_annotation =
    dynamic_cast<ExternalAnnotation*>(
      builtin_annotations_["::@external"]);
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(
      builtin_annotations_["::@try_construct"]);

  for (AST_Annotation_Appls::iterator i = node->annotations().begin();
       i != node->annotations().end(); ++i) {
    AST_Annotation_Appl* appl = i->get();
    if (appl &&
        appl->annotation_decl() != external_annotation->declaration() &&
        appl->annotation_decl() != try_construct_annotation->declaration()) {
      return false;
    }
  }

  return true;
}

TryConstructFailAction BE_GlobalData::try_construct(AST_Decl* node) const
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(
      builtin_annotations_["::@try_construct"]);
  TryConstructFailAction value;
  if (!try_construct_annotation->node_value_exists(node, value)) {
    value = default_try_construct_;
  }
  return value;
}

TryConstructFailAction BE_GlobalData::sequence_element_try_construct(AST_Sequence* node)
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(builtin_annotations_["::@try_construct"]);
  return try_construct_annotation->sequence_element_value(node);
}

TryConstructFailAction BE_GlobalData::array_element_try_construct(AST_Array* node)
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(builtin_annotations_["::@try_construct"]);
  return try_construct_annotation->array_element_value(node);
}

TryConstructFailAction BE_GlobalData::union_discriminator_try_construct(AST_Union* node)
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(builtin_annotations_["::@try_construct"]);
  return try_construct_annotation->union_value(node);
}

#if OPENDDS_HAS_IDL_MAP
TryConstructFailAction BE_GlobalData::map_key_try_construct(AST_Map* node)
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(builtin_annotations_["::@try_construct"]);
  return try_construct_annotation->map_key(node);
}

TryConstructFailAction BE_GlobalData::map_value_try_construct(AST_Map* node)
{
  TryConstructAnnotation* try_construct_annotation =
    dynamic_cast<TryConstructAnnotation*>(builtin_annotations_["::@try_construct"]);
  return try_construct_annotation->map_value(node);
}
#endif

OpenDDS::DataRepresentation BE_GlobalData::data_representations(
  AST_Decl* node) const
{
  using namespace OpenDDS;
  DataRepresentationAnnotation* data_representation_annotation =
    dynamic_cast<DataRepresentationAnnotation*>(
      builtin_annotations_["::OpenDDS::@data_representation"]);
  DataRepresentation value;
  if (!data_representation_annotation->node_value_exists(node, value)) {
    value = default_data_representation_;
  }
  return value;
}

OpenDDS::XTypes::MemberId BE_GlobalData::compute_id(
  AST_Structure* stru, AST_Field* field, AutoidKind auto_id, OpenDDS::XTypes::MemberId& member_id)
{
  const MemberIdMap::const_iterator pos = member_id_map_.find(field);
  if (pos != member_id_map_.end()) {
    return pos->second;
  }

  using OpenDDS::XTypes::hash_member_name_to_id;
  using OpenDDS::XTypes::MemberId;
  const string field_name = canonical_name(field);
  string hash_id;
  MemberId mid;
  if (id(field, member_id)) {
    // @id
    mid = member_id++;
  } else if (hashid(field, hash_id)) {
    // @hashid
    mid = hash_member_name_to_id(hash_id.empty() ? field_name : hash_id);
  } else if (auto_id == autoidkind_hash) {
    mid = hash_member_name_to_id(field_name);
  } else {
    // auto_id == autoidkind_sequential
    mid = member_id++;
  }

  // Check for collision with ids in the same type
  GlobalMemberIdCollisionMap::iterator git = member_id_collision_map_.find(stru);
  if (git == member_id_collision_map_.end()) {
    git = member_id_collision_map_.insert(
      std::pair<AST_Structure*, MemberIdCollisionMap>(stru, MemberIdCollisionMap())).first;
  }
  MemberIdCollisionMap::iterator lit = git->second.find(mid);
  if (lit != git->second.end()) {
    std::ostringstream msg;
    msg << "Member id " << mid << " is the same as on field " << canonical_name(lit->second);
    be_util::misc_error_and_abort(msg.str(), field);
  }
  git->second.insert(std::pair<MemberId, AST_Field*>(mid, field));

  if (mid > OpenDDS::DCPS::Serializer::MEMBER_ID_MAX) {
    std::ostringstream msg;
    msg << "Member id " << mid << " exceeds the maximum allowed value (" <<
      OpenDDS::DCPS::Serializer::MEMBER_ID_MAX << ")";
    be_util::misc_error_and_abort(msg.str(), field);
  }
  member_id_map_[field] = mid;
  return mid;
}

OpenDDS::XTypes::MemberId BE_GlobalData::get_id(AST_Field* field)
{
  const MemberIdMap::const_iterator pos = member_id_map_.find(field);
  if (pos != member_id_map_.end()) {
    return pos->second;
  }
  be_util::misc_error_and_abort("Could not get member id for field");
  return -1;
}

bool BE_GlobalData::dynamic_data_adapter(AST_Decl* node) const
{
  return !builtin_annotations_["::OpenDDS::internal::@no_dynamic_data_adapter"]->find_on(node);
}

bool BE_GlobalData::special_serialization(AST_Decl* node, std::string& template_name) const
{
  typedef OpenDDS::internal::SpecialSerializationAnnotation Anno;
  const Anno* const anno =
    dynamic_cast<const Anno*>(builtin_annotations_["::OpenDDS::internal::@special_serialization"]);
  if (!anno->node_value_exists(node, template_name)) {
    return false;
  }
  if (template_name.empty()) {
    template_name = canonical_name(node->local_name());
  }
  return true;
}
