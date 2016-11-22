/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_global.h"
#include "be_util.h"
#include "ast_generator.h"
#include "global_extern.h"
#include "idl_defines.h"
#include "utl_err.h"
#include "utl_string.h"

#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/ARGV.h"

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
  , generate_wireshark_(false)
  , generate_itl_(false)
  , v8_(false)
  , face_ts_(false)
  , seq_("Seq")
  , language_mapping_(LANGMAP_NONE)
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

void BE_GlobalData::v8(bool b)
{
  this->v8_ = b;
}

bool BE_GlobalData::v8() const
{
  return this->v8_;
}

void BE_GlobalData::face_ts(bool b)
{
  this->face_ts_ = b;
}

bool BE_GlobalData::face_ts() const
{
  return this->face_ts_;
}

// bool
// BE_GlobalData::do_included_files() const
// {
//   return false; //we never process included files
// }

void
BE_GlobalData::open_streams(const char* filename)
{
  this->filename(filename);
  size_t len = strlen(filename);

  if ((len < 5 || 0 != ACE_OS::strcasecmp(filename + len - 4, ".idl"))
      && (len < 6 || 0 != ACE_OS::strcasecmp(filename + len - 5, ".pidl"))) {
    UTL_Error u;
    UTL_String str("Input filename must end in \".idl\" or \".pidl\".");
    u.back_end(0, &str);
    exit(-1);
    return;
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
  ws_config_name_ = (filebase + "_ws.ini").c_str();
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

void
BE_GlobalData::parse_args(long& i, char** av)
{
  static const char WB_EXPORT_MACRO[] = "--export=";
  static const size_t SZ_WB_EXPORT_MACRO = sizeof(WB_EXPORT_MACRO) - 1;

  switch (av[i][1]) {
  case 'o':
    idl_global->append_idl_flag(av[i + 1]);
    if (ACE_OS::mkdir(av[i + 1]) != 0 && errno != EEXIST) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("IDL: unable to create directory %C")
        ACE_TEXT(" specified by -o option\n"), av[i + 1]));
    } else {
      output_dir_ = av[++i];
    }
    break;
  case 'G':
    if (0 == ACE_OS::strcmp(av[i], "-Gws"))
      generate_wireshark_ = true;
    else if (0 == ACE_OS::strcmp(av[i], "-Gitl"))
      generate_itl_ = true;
    else if (0 == ACE_OS::strcasecmp(av[i], "-GfaceTS"))
      face_ts(true);
    else
      {
        ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C'")
                   ACE_TEXT(" option\n"), av[i]));
        idl_global->set_compile_flags(idl_global->compile_flags()
                                      | IDL_CF_ONLY_USAGE);
      }
    break;
  case 'L':
    if (0 == ACE_OS::strcasecmp(av[i], "-Lface"))
      language_mapping(LANGMAP_FACE_CXX);
    else if (0 == ACE_OS::strcasecmp(av[i], "-Lspcpp"))
      language_mapping(LANGMAP_SP_CXX);
    else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C'")
                  ACE_TEXT(" option\n"), av[i]));
      idl_global->set_compile_flags(idl_global->compile_flags()
                                    | IDL_CF_ONLY_USAGE);
    }
    break;
  case 'S':
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C'")
        ACE_TEXT(" option\n"), av[i]));
      idl_global->set_compile_flags(idl_global->compile_flags()
                                    | IDL_CF_ONLY_USAGE);
    }
    break;
  case 'Z':
    switch (av[i][2]) {
    case 'C':
      add_include (av[++i], STREAM_CPP);
      break;
    default:
      ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C'")
                 ACE_TEXT(" option\n"), av[i]));
      idl_global->set_compile_flags(idl_global->compile_flags()
                                    | IDL_CF_ONLY_USAGE);
    }
    break;
  case '-':
    if (0 == ACE_OS::strncasecmp(av[i], WB_EXPORT_MACRO, SZ_WB_EXPORT_MACRO)) {
      this->export_macro(av[i] + SZ_WB_EXPORT_MACRO);
    }
    break;
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C' option\n"),
               av[i]));
    idl_global->set_compile_flags(idl_global->compile_flags()
                                  | IDL_CF_ONLY_USAGE);
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
    std::for_each(referenced_idl_.begin(), referenced_idl_.end(),
                  InsertRefIncludes(ret, "TypeSupportImpl.h"));
    break;
  default:
    break;
  }

  return ret.str();
}
