/*
 * $Id$
 */

#include <iostream>
#include <string>

#include "be_global.h"

namespace
{
  class BE_Arg {
  public:
    BE_Arg(const std::string &s)
    {
      size_t pos = s.find('=');
      this->name_ = s.substr(0, pos);

      if (pos != std::string::npos) {
        this->value_ = s.substr(pos + 1);
      }
    }
    
    ~BE_Arg()
    {
    }

    std::string name(void) const
    {
      return this->name_;
    }
    
    std::string value(void) const
    {
      return this->value_;
    }

  private:
    std::string name_;
    std::string value_;
  };
}

BE_GlobalData *be_global = 0;

BE_GlobalData::BE_GlobalData()
  : output_otp_ (false)
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
BE_GlobalData::parse_args(long &, char **)
{
}

void
BE_GlobalData::prep_be_arg(char *arg_)
{
  BE_Arg arg (arg_);

  if ("stub_export_include" == arg.name()) {
    this->stub_export_include_ = arg.value();

  } else if ("stub_export_macro" == arg.name()) {
    this->stub_export_macro_ = arg.value();
  
  } else {
    std::cerr
      << "warning: unknown argument: " << arg.name()
      << std::endl;
  }
}

void
BE_GlobalData::arg_post_proc()
{
}

void
BE_GlobalData::usage() const
{
  std::cerr
    << " -o <output_dir>\tOutput directory for the generated files. Default"
    << " is current directory"
    << std::endl;

  std::cerr
    << " -otp\t\t\tOutput directory uses the OTP layout. Generated files will"
    << " be created in src, include, and cpp_src under <output_dir>"
    << std::endl;

  std::cerr 
    << " -Wb,stub_export_macro=<macro name>\t\tsets export macro for client"
    << " files only"
    << std::endl;

  std::cerr
    << " -Wb,stub_export_include=<include path>\t\tsets export include file"
    << " for client only"
    << std::endl;
}

AST_Generator *
BE_GlobalData::generator_init()
{
  return new(std::nothrow) AST_Generator;
}

std::string
BE_GlobalData::stub_export_include() const
{
  return this->stub_export_include_;
}

void
BE_GlobalData::stub_export_include(const std::string &stub_export_include)
{
  this->stub_export_include_ = stub_export_include;
}

std::string
BE_GlobalData::stub_export_macro() const
{
  return this->stub_export_macro_;
}

void
BE_GlobalData::stub_export_macro(const std::string &stub_export_macro)
{
  this->stub_export_macro_ = stub_export_macro;
}

std::string
BE_GlobalData::output_dir() const
{
  return this->output_dir_;
}

void
BE_GlobalData::output_dir(const std::string &output_dir)
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
