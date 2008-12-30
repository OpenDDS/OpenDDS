/*
 * $Id$
 */

#ifndef TAO_IC_BE_GLOBAL_H
#define TAO_IC_BE_GLOBAL_H

#ifndef ACE_LACKS_PRAGMA_ONCE
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Basic_Types.h"

#include "ast_generator.h"

class BE_GlobalData
{
public:
  BE_GlobalData(void);
  ~BE_GlobalData(void);

  void destroy(void);
  // Cleanup function.

  void parse_args(long &, char **);
  // Parse args that affect the backend.

  void prep_be_arg(char *);
  // Special BE arg call factored out of DRV_args.

  void arg_post_proc(void);
  // Checks made after parsing args.

  void usage(void) const;
  // Display usage of BE-specific options.

  AST_Generator *generator_init(void);
  // Create an AST node generator.

  /// BE-specific methods
  ACE_CString output_dir(void) const;
  void output_dir(const ACE_CString &);

  bool output_otp(void) const;
  void output_otp(bool);

  ACE_CString port_driver_name(void) const;
  void port_driver_name(const ACE_CString &);

  ACE_CString skel_export_include(void) const;
  void skel_export_include(const ACE_CString &);

  ACE_CString skel_export_macro(void) const;
  void skel_export_macro(const ACE_CString &);

  ACE_CString stub_export_include(void) const;
  void stub_export_include(const ACE_CString &);

  ACE_CString stub_export_macro(void) const;
  void stub_export_macro(const ACE_CString &);

  bool suppress_skel(void) const;
  void suppress_skel(bool);

private:
  ACE_CString output_dir_;
  // -o <output_dir>

  bool output_otp_;
  // -otp

  ACE_CString port_driver_name_;
  // -Wb,port_driver_name=<driver_name>

  ACE_CString skel_export_include_;
  // -Wb,skel_export_include=<include path>

  ACE_CString skel_export_macro_;
  // -Wb,skel_export_macro=<macro name>
  
  ACE_CString stub_export_include_;
  // -Wb,stub_export_include=<include path>
  
  ACE_CString stub_export_macro_;
  // -Wb,stub_export_macro=<macro name>
  
  bool suppress_skel_;
  // -SS
};

#endif /* TAO_IC_BE_GLOBAL_H */
