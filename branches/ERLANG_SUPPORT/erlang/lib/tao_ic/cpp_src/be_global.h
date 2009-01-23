/*
 * $Id$
 */

#ifndef TAO_IC_BE_GLOBAL_H
#define TAO_IC_BE_GLOBAL_H

#include "ast_generator.h"

class BE_GlobalData
{
public:
  BE_GlobalData(void);

  ~BE_GlobalData(void);

  void destroy(void);

  void parse_args(long& ac, char** av);

  void prep_be_arg(char* s);

  void arg_post_proc(void);

  void usage(void) const;

  AST_Generator *generator_init(void);

  void get_include_dir(std::string& s) const;

  void get_src_dir(std::string& s) const;

  void get_cpp_src_dir(std::string& s) const;

  std::string output_dir(void) const;

  bool output_otp(void) const;

  std::string port_driver_name(void) const;

  std::string skel_export_include(void) const;

  std::string skel_export_macro(void) const;

  std::string stub_export_include(void) const;

  std::string stub_export_macro(void) const;

  bool suppress_skel(void) const;

private:
  std::string output_dir_;
  bool output_otp_;

  std::string port_driver_name_;

  std::string skel_export_include_;
  std::string skel_export_macro_;

  std::string stub_export_include_;
  std::string stub_export_macro_;

  bool suppress_skel_;

  void get_output_dir(std::string& s, const char* dirname) const;
};

#endif /* TAO_IC_BE_GLOBAL_H */
