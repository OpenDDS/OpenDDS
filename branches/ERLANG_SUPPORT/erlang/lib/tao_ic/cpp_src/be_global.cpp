/*
 * $Id$
 */

#include "ace/Arg_Shifter.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"

#include "be_global.h"

namespace {
static const char *DEFAULT_PORT_DRIVER_NAME = "port_driver";

class BE_Arg {
public:
  explicit BE_Arg(const ACE_CString &s)
  {
    ACE_Allocator::size_type pos = s.find('=');
    this->name_ = s.substring(0, pos);

    if (pos != ACE_CString::npos) {
      this->value_ = s.substring(pos + 1);
    }
  }

  ~BE_Arg()
  {
  }

  ACE_CString name(void) const { return this->name_; }
  ACE_CString value(void) const { return this->value_; }

private:
  ACE_CString name_;
  ACE_CString value_;
};

bool operator==(const BE_Arg &arg, const char *s)
{ 
  return arg.name() == s;
}

bool operator==(const char *s, const BE_Arg &arg)
{ 
  return s == arg.name();
}
} // namespace

BE_GlobalData *be_global = 0;

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
BE_GlobalData::parse_args(long &argc_, char **argv)
{
  int argc = static_cast<int>(argc_);
  
  ACE_Arg_Shifter_T<char> shifter(argc, argv);
  while (shifter.is_anything_left()) {
    const char *arg = 0;

    // -o <output_dir>
    if ((arg = shifter.get_the_parameter("-o")) != 0) {
      this->output_dir_ = arg;

    // -otp
    } else if (shifter.cur_arg_strncasecmp("-otp") == 0) {
      this->output_otp_ = true;

    // -SS
    } else if (shifter.cur_arg_strncasecmp("-SS") == 0) {
      this->suppress_skel_ = true;
    
    } else {
      shifter.ignore_arg();
    }
  }
  argc_ = argc; // update reference
}

void
BE_GlobalData::prep_be_arg(char *arg_)
{
  BE_Arg arg(arg_);

  // -Wb,port_driver_name=<driver_name>
  if ("port_driver_name" == arg) {
    this->port_driver_name_ = arg.value();

  // -Wb,skel_export_include=<include path>
  } else if ("skel_export_include" == arg) {
    this->skel_export_include_ = arg.value();

  // -Wb,skel_export_macro=<macro name>
  } else if ("skel_export_macro" == arg) {
    this->skel_export_macro_ = arg.value();

  // -Wb,stub_export_include=<include path>
  } else if ("stub_export_include" == arg) {
    this->stub_export_include_ = arg.value();

  // -Wb,stub_export_macro=<macro name>
  } else if ("stub_export_macro" == arg) {
    this->stub_export_macro_ = arg.value();

  } else {
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

AST_Generator *
BE_GlobalData::generator_init()
{
  AST_Generator *gen;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

ACE_CString
BE_GlobalData::output_dir() const
{
  return this->output_dir_;
}

void
BE_GlobalData::output_dir(const ACE_CString &output_dir)
{
  this->output_dir_ = output_dir;
}

bool
BE_GlobalData::output_otp() const
{
  return this->output_otp_;
}

void
BE_GlobalData::output_otp(bool output_otp)
{
  this->output_otp_ = output_otp;
}

ACE_CString
BE_GlobalData::port_driver_name() const
{
  return this->port_driver_name_;
}

void
BE_GlobalData::port_driver_name(const ACE_CString &port_driver_name)
{
  this->port_driver_name_ = port_driver_name;
}

ACE_CString
BE_GlobalData::skel_export_include() const
{
  return this->skel_export_include_;
}

void
BE_GlobalData::skel_export_include(const ACE_CString &skel_export_include)
{
  this->skel_export_include_ = skel_export_include;
}

ACE_CString
BE_GlobalData::skel_export_macro() const
{
  return this->skel_export_macro_;
}

void
BE_GlobalData::skel_export_macro(const ACE_CString &skel_export_macro)
{
  this->skel_export_macro_ = skel_export_macro;
}

ACE_CString
BE_GlobalData::stub_export_include() const
{
  return this->stub_export_include_;
}

void
BE_GlobalData::stub_export_include(const ACE_CString &stub_export_include)
{
  this->stub_export_include_ = stub_export_include;
}

ACE_CString
BE_GlobalData::stub_export_macro() const
{
  return this->stub_export_macro_;
}

void
BE_GlobalData::stub_export_macro(const ACE_CString &stub_export_macro)
{
  this->stub_export_macro_ = stub_export_macro;
}
