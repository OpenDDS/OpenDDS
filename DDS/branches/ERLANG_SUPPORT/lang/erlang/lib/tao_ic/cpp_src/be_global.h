/*
 * $Id$
 */

#ifndef BE_GLOBAL_H
#define BE_GLOBAL_H

#include <string>

#include "ast_generator.h"

class BE_GlobalData
{
public:
  BE_GlobalData();

  ~BE_GlobalData();

  void destroy();

  void parse_args(long& ac, char** av);

  void prep_be_arg(char* s);

  void arg_post_proc();

  void usage() const;

  AST_Generator *generator_init();

  void get_include_dir(std::string& s) const;

  void get_src_dir(std::string& s) const;

  void get_cpp_src_dir(std::string& s) const;
  
  void get_output_dir(std::string& s, const char* dirname) const;

  std::string output_dir() const;

  bool output_otp() const;

  std::string port_driver_name() const;

  std::string skel_export_include() const;

  std::string skel_export_macro() const;

  std::string stub_export_include() const;

  std::string stub_export_macro() const;

  bool suppress_skel() const;

private:
  std::string output_dir_;

  bool output_otp_;

  std::string port_driver_name_;

  std::string skel_export_include_;
  std::string skel_export_macro_;

  std::string stub_export_include_;
  std::string stub_export_macro_;

  bool suppress_skel_;
};

#endif /* BE_GLOBAL_H */
