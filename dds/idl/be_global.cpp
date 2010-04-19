/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_global.h"
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

BE_GlobalData* be_global = 0;

BE_GlobalData::BE_GlobalData()
  : filename_(0), java_(false)
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

void BE_GlobalData::java(bool b)
{
  this->java_ = b;
}

bool BE_GlobalData::java() const
{
  return this->java_;
}


bool
BE_GlobalData::do_included_files() const
{
  return false; //we never process included files
}

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
  size_t idx = filebase.rfind(ACE_DIRECTORY_SEPARATOR_CHAR);

  if (idx != string::npos) {
    filebase = filebase.substr(idx + 1);
  }

  header_name_ = (filebase + "TypeSupportImpl.h").c_str();
  impl_name_ = (filebase + "TypeSupportImpl.cpp").c_str();
  idl_name_ = (filebase + "TypeSupport.idl").c_str();
}


void
BE_GlobalData::multicast(const char* str)
{
  header_ << str;
  impl_ << str;
  idl_ << str;
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
  ACE_ERROR((LM_ERROR, ACE_TEXT("IDL: I don't understand the '%C' option\n"),
             av[i]));

  idl_global->set_compile_flags(idl_global->compile_flags()
                                | IDL_CF_ONLY_USAGE);
}

void
BE_GlobalData::prep_be_arg(char* arg)
{
  static const char WB_EXPORT_MACRO[] = "export_macro=";
  static const size_t SZ_WB_EXPORT_MACRO = sizeof(WB_EXPORT_MACRO) - 1;
  static const char WB_EXPORT_INCLUDE[] = "export_include=";
  static const size_t SZ_WB_EXPORT_INCLUDE = sizeof(WB_EXPORT_INCLUDE) - 1;
  static const char WB_PCH_INCLUDE[] = "pch_include=";
  static const size_t SZ_WB_PCH_INCLUDE = sizeof(WB_PCH_INCLUDE) - 1;
  static const char WB_JAVA[] = "java";
  static const size_t SZ_WB_JAVA = sizeof(WB_JAVA) - 1;

  if (0 == ACE_OS::strncasecmp(arg, WB_EXPORT_MACRO, SZ_WB_EXPORT_MACRO)) {
    this->export_macro(arg + SZ_WB_EXPORT_MACRO);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_EXPORT_INCLUDE,
                                      SZ_WB_EXPORT_INCLUDE)) {
    this->export_include(arg + SZ_WB_EXPORT_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_PCH_INCLUDE, SZ_WB_PCH_INCLUDE)) {
    this->pch_include(arg + SZ_WB_PCH_INCLUDE);

  } else if (0 == ACE_OS::strncasecmp(arg, WB_JAVA, SZ_WB_JAVA)) {
    this->java(true);
    if (ACE_OS::strlen(arg + SZ_WB_JAVA)) {
      this->java_arg(arg + SZ_WB_JAVA + 1 /* = */);
    }
  }
}

// Does nothing in this backend.
void
BE_GlobalData::arg_post_proc()
{
}

void
BE_GlobalData::usage() const
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -Wb,export_macro=<macro name>\t\tsets export macro ")
    ACE_TEXT("for all files\n")
  ));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -Wb,export_include=<include path>\tsets export include ")
    ACE_TEXT("file for all files\n")
  ));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -Wb,pch_include=<include path>\t\tsets include ")
    ACE_TEXT("file for precompiled header mechanism\n")
  ));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" -Wb,java[=<output_file>]\t\tenables Java support ")
    ACE_TEXT("for TypeSupport files.  Do not specify an 'output_file' ")
    ACE_TEXT("except for special cases.\n")
  ));
}

AST_Generator* 
BE_GlobalData::generator_init()
{
  AST_Generator* gen = 0;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

bool
BE_GlobalData::writeFile(const char* fileName, const string& content)
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
  Includes_t inc_h_, inc_c_, inc_idl_;
}

void
BE_GlobalData::reset_includes()
{
  inc_h_.clear();
  inc_c_.clear();
  inc_idl_.clear();
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
  default:
    return;
  }

  inc->insert(file);
}

ACE_CString
BE_GlobalData::get_include_block(BE_GlobalData::stream_enum_t which)
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
  default:
    return "";
  }

  ACE_CString ret;
  Includes_t::const_iterator it = inc->begin(), end = inc->end();

  for (; it != end; ++it) {
    ret += "#include \"" + ACE_CString(it->c_str()) + "\"\n";
  }

  if (which == STREAM_H) {
    ACE_CString exports = this->export_include();

    if (exports != "")
      ret += "#include \"" + exports + "\"\n";

  }

  return ret;
}
