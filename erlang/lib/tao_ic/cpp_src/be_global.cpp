/*
 * $Id$
 */

#include "ace/Arg_Shifter.h"
#include "ace/Basic_Types.h"
#include "ace/Log_Msg.h"

#include "be_global.h"

namespace
{
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

    ACE_CString 
    name(void) const
    {
      return this->name_;
    }

    ACE_CString
    value(void) const
    {
      return this->value_;
    }

    bool
    operator==(const char *s) const
    {
      return s == this->name_;
    }

  private:
    ACE_CString name_;
    ACE_CString value_;
  };
}

BE_GlobalData *be_global = 0;

BE_GlobalData::BE_GlobalData()
  : output_otp_(false),
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

    if ((arg = shifter.get_the_parameter("-o")) != 0) {
      this->output_dir_ = arg;

    } else if (shifter.cur_arg_strncasecmp("-otp") == 0) {
      this->output_otp_ = true;

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

  if (arg == "skel_export_include") {
    this->skel_export_include_ = arg.value();

  } else if (arg == "skel_export_macro") {
    this->skel_export_macro_ = arg.value();

  } else if (arg == "stub_export_include") {
    this->stub_export_include_ = arg.value();

  } else if (arg == "stub_export_macro") {
    this->stub_export_macro_ = arg.value();

  } else {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("ignoring argument: %s\n"),
               ACE_TEXT(arg_)));
  }
}

void
BE_GlobalData::arg_post_proc()
{
}

void
BE_GlobalData::usage() const
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -o <output_dir>\tOutput directory for the generated")
             ACE_TEXT(" files. Default is current directory\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -otp\t\t\tOutput directory uses the OTP layout.")
             ACE_TEXT(" Generated files will be created in src, include, and")
             ACE_TEXT(" cpp_src under <output_dir>\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -Wb,skel_export_macro=<macro name>\t\tsets export")
             ACE_TEXT(" macro for server files only\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -Wb,skel_export_include=<include path>\t\tsets")
             ACE_TEXT(" export include file for server only\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -Wb,stub_export_macro=<macro name>\t\tsets export")
             ACE_TEXT(" macro for client files only\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -Wb,stub_export_include=<include path>\t\tsets")
             ACE_TEXT(" export include file for client only\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" -SS\t\t\tsuppress generating skeleton implementation")
             ACE_TEXT(" and inline file (disabled by default)\n")));
}

AST_Generator *
BE_GlobalData::generator_init()
{
  return new(std::nothrow) AST_Generator;
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
