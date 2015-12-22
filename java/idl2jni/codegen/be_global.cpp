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

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

using namespace std;

BE_GlobalData *be_global = 0;

BE_GlobalData::BE_GlobalData()
  : filename_(0),
    do_server_side_(true)
{
  // At this point, the FE has been initialized.  We can
  // now instruct it that we want to preserve c++ keywords.
  idl_global->preserve_cpp_keywords(true);
}

BE_GlobalData::~BE_GlobalData()
{
}

void
BE_GlobalData::destroy()
{
}

const char *
BE_GlobalData::filename() const
{
  return this->filename_;
}

ACE_CString BE_GlobalData::stub_export_macro() const
{
  return this->stub_export_macro_;
}

void BE_GlobalData::stub_export_macro(const ACE_CString &str)
{
  this->stub_export_macro_ = str;
}

ACE_CString BE_GlobalData::stub_export_include() const
{
  return this->stub_export_include_;
}

void BE_GlobalData::stub_export_include(const ACE_CString &str)
{
  this->stub_export_include_ = str;
}

ACE_CString BE_GlobalData::skel_export_macro() const
{
  return this->skel_export_macro_;
}

void BE_GlobalData::skel_export_macro(const ACE_CString &str)
{
  this->skel_export_macro_ = str;
}

ACE_CString BE_GlobalData::skel_export_include() const
{
  return this->skel_export_include_;
}

void BE_GlobalData::skel_export_include(const ACE_CString &str)
{
  this->skel_export_include_ = str;
}

ACE_CString BE_GlobalData::native_lib_name() const
{
  return this->native_lib_name_;
}

void BE_GlobalData::native_lib_name(const ACE_CString &str)
{
  this->native_lib_name_ = str;
}

void
BE_GlobalData::filename(const char *fname)
{
  this->filename_ = fname;
}

bool
BE_GlobalData::do_included_files() const
{
  return false; //we never process included files
}

bool
BE_GlobalData::do_server_side() const
{
  return this->do_server_side_;
}

void
BE_GlobalData::do_server_side(bool val)
{
  this->do_server_side_ = val;
}

void
BE_GlobalData::open_streams(const char *filename)
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
  size_t idx = filebase.rfind(ACE_DIRECTORY_SEPARATOR_CHAR);

  if (idx != string::npos) {
    filebase = filebase.substr(idx + 1);
  }

  stub_header_name_ = (filebase + "JC.h").c_str();
  stub_impl_name_ = (filebase + "JC.cpp").c_str();

  if (this->do_server_side()) {
    skel_header_name_ = (filebase + "JS.h").c_str();
    skel_impl_name_ = (filebase + "JS.cpp").c_str();
  }
}

void
BE_GlobalData::close_streams()
{
}

void
BE_GlobalData::multicast(const char *str)
{
  stub_header_ << str;
  stub_impl_ << str;

  if (this->do_server_side()) {
    skel_header_ << str;
    skel_impl_ << str;
  }
}

BE_Comment_Guard::BE_Comment_Guard(const char *type, const char *name)
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
  return /*this->orb_args_ +*/ idl_global->idl_flags();
}

void
BE_GlobalData::parse_args(long &i, char **av)
{
  switch (av[i][1]) {
  case 'S':

    // Suppress ...
    /*
      if (av[i][2] == 'i')
      {
      // ... processing of included IDL files.
      be_global->do_included_files (0);
      }
      else*/
    if (av[i][2] == 'S') {
      this->do_server_side(0);

    } else {
      ACE_ERROR((
                  LM_ERROR,
                  ACE_TEXT("IDL: I don't understand the '%s' option\n"),
                  av[i]));
      ACE_OS::exit(99);
    }

    break;
  default:
    ACE_ERROR((
                LM_ERROR,
                ACE_TEXT("IDL: I don't understand the '%s' option\n"),
                av[i]));

    idl_global->set_compile_flags(idl_global->compile_flags()
                                  | IDL_CF_ONLY_USAGE);
    break;
  }
}


bool
BE_GlobalData::writeFile(const char *fileName, const string &content)
{
  ofstream ofs(fileName);

  if (!ofs) {
    cerr << "ERROR - couldn't open " << fileName << " for writing.\n";
    return false;
  }

  ofs << content;
  return !!ofs;
}

//include file management (assumes a singleton BE_GlobalData object)

namespace {
typedef set<string> Includes_t;
Includes_t ch_, cc_, sh_, sc_;
}

void
BE_GlobalData::reset_includes()
{
  ch_.clear();
  cc_.clear();
  sh_.clear();
  sc_.clear();
}

void
BE_GlobalData::add_include(const char *file,
                           BE_GlobalData::stream_enum_t which)
{
  Includes_t *inc = 0;

  switch (which) {
  case STUB_H:
    inc = &ch_;
    break;
  case STUB_CPP:
    inc = &cc_;
    break;
  case SKEL_H:
    inc = &sh_;
    break;
  case SKEL_CPP:
    inc = &sc_;
    break;
  default:
    return;
  }

  inc->insert(file);
}

ACE_CString
BE_GlobalData::get_include_block(BE_GlobalData::stream_enum_t which)
{
  Includes_t *inc = 0;

  switch (which) {
  case STUB_H:
    inc = &ch_;
    break;
  case STUB_CPP:
    inc = &cc_;
    break;
  case SKEL_H:
    inc = &sh_;
    break;
  case SKEL_CPP:
    inc = &sc_;
    break;
  default:
    return "";
  }

  ACE_CString ret;
  Includes_t::const_iterator it = inc->begin(), end = inc->end();

  for (; it != end; ++it) {
    ret += "#include \"" + ACE_CString(it->c_str()) + "\"\n";
  }

  if (which == STUB_H) {
    ACE_CString exports = this->stub_export_include();

    if (exports != "")
      ret += "#include \"" + exports + "\"\n";

  } else if (which == SKEL_H) {
    ACE_CString exports = this->skel_export_include();

    if (exports != "")
      ret += "#include \"" + exports + "\"\n";
  }

  return ret;
}
