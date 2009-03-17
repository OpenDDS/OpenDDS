/*
 * $Id$
 */

#include <cstring>

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"

#include "ace_compat.h"
#include "be_global.h"

using namespace std;

BE_GlobalData* be_global = 0;

namespace
{
const char* DEFAULT_PORT_DRIVER_NAME = "port_driver";

class BE_Arg
{
public:
  explicit BE_Arg(const string& s)
  {
    size_t pos = s.find('=');
    name_ = s.substr(0, pos);

    if (pos != string::npos)
    {
      value_ = s.substr(pos + 1);
    }
  }

  ~BE_Arg()
  {
  }

  string name() const { return name_; }

  string value() const { return value_; }

private:
  string name_;
  string value_;

  friend bool
  operator==(const BE_Arg& lhs, const char* rhs)
  {
    return lhs.name_ == rhs;
  }

  friend bool
  operator==(const char* lhs, const BE_Arg& rhs)
  {
    return lhs == rhs.name_;
  }
};

void
normalize_path(string& s)
{
  size_t len = s.length();
  if (len > 0 && s[len - 1] != ACE_DIRECTORY_SEPARATOR_CHAR_A)
  {
    s.push_back(ACE_DIRECTORY_SEPARATOR_CHAR_A);
  }
}
} // namespace

BE_GlobalData::BE_GlobalData()
  : output_otp_(false),
    port_driver_name_(DEFAULT_PORT_DRIVER_NAME),
    suppress_skel_(false)
{
}

BE_GlobalData::~BE_GlobalData()
{
}

void
BE_GlobalData::destroy()
{
}

void
BE_GlobalData::parse_args(long& ac, char** av)
{
  const char* a = av[ac];

  if (strcmp("-o", a) == 0)
  {
    output_dir_ = av[++ac];
    normalize_path(output_dir_);
  }
  else if (strcmp("-otp", a) == 0)
  {
    output_otp_ = true;
  }
  else if (strcmp("-SS", a) == 0)
  {
    suppress_skel_ = true;
  }
}

void
BE_GlobalData::prep_be_arg(char* s)
{
  BE_Arg arg(s);

  if ("port_driver_name" == arg)
  {
    port_driver_name_ = arg.value();
  }
  else if ("skel_export_include" == arg)
  {
    skel_export_include_ = arg.value();
  }
  else if ("skel_export_macro" == arg)
  {
    skel_export_macro_ = arg.value();
  }
  else if ("stub_export_include" == arg)
  {
    stub_export_include_ = arg.value();
  }
  else if ("stub_export_macro" == arg)
  {
    stub_export_macro_ = arg.value();
  }
  else
  {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("%N:%l: prep_be_arg()")
               ACE_TEXT(" ignoring unknown argument: %s\n"),
               ACE_TEXT(arg.name().c_str())));
  }
}

void
BE_GlobalData::arg_post_proc()
{
}

void
BE_GlobalData::usage() const
{
  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -o <output_dir>\tOutput directory for the generated")
             ACE_TEXT(" files. Default is current directory\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -otp\t\t\tOutput directory uses the OTP layout.")
             ACE_TEXT(" Generated files will be created in src, include, and")
             ACE_TEXT(" cpp_src under <output_dir>\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -Wb,port_driver_name=<driver_name>\t\tsets port")
             ACE_TEXT(" driver name used to load the native runtime")
             ACE_TEXT(" (default is %s)\n"),
             ACE_TEXT(DEFAULT_PORT_DRIVER_NAME)));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -Wb,skel_export_macro=<macro name>\t\tsets export")
             ACE_TEXT(" macro for server files only\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -Wb,skel_export_include=<include path>\t\tsets")
             ACE_TEXT(" export include file for server only\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -Wb,stub_export_macro=<macro name>\t\tsets export")
             ACE_TEXT(" macro for client files only\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -Wb,stub_export_include=<include path>\t\tsets")
             ACE_TEXT(" export include file for client only\n")));

  ACE_ERROR((LM_INFO,
             ACE_TEXT(" -SS\t\t\tsuppress generating skeleton implementation")
             ACE_TEXT(" and inline file (disabled by default)\n")));
}

AST_Generator*
BE_GlobalData::generator_init()
{
  AST_Generator* gen;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

void
BE_GlobalData::get_include_dir(string& s) const
{
  get_output_dir(s, "include");
}

void
BE_GlobalData::get_src_dir(string& s) const
{
  get_output_dir(s, "src");
}

void
BE_GlobalData::get_cpp_src_dir(string& s) const
{
  get_output_dir(s, "cpp_src");
}

void
BE_GlobalData::get_output_dir(string& s, const char* dirname) const
{
  s.append(output_dir_);
  if (output_otp_)
  {
    s.append(dirname);
  }
  normalize_path(s);
}

string
BE_GlobalData::output_dir() const
{
  return output_dir_;
}

bool
BE_GlobalData::output_otp() const
{
  return output_otp_;
}

string
BE_GlobalData::port_driver_name() const
{
  return port_driver_name_;
}

string
BE_GlobalData::skel_export_include() const
{
  return skel_export_include_;
}

string
BE_GlobalData::skel_export_macro() const
{
  return skel_export_macro_;
}

string
BE_GlobalData::stub_export_include() const
{
  return stub_export_include_;
}

string
BE_GlobalData::stub_export_macro() const
{
  return stub_export_macro_;
}
