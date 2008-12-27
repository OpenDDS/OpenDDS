/*
 * $Id$
 */

#ifndef TAO_IC_BE_GLOBAL_H
#define TAO_IC_BE_GLOBAL_H

#ifndef ACE_LACKS_PRAGMA_ONCE
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
  std::string output_dir(void) const;
  void output_dir(const std::string &);

  bool output_otp(void) const;
  void output_otp(bool);

  std::string stub_export_include(void) const;
  void stub_export_include(const std::string &);

  std::string stub_export_macro(void) const;
  void stub_export_macro(const std::string &);

private:
  std::string output_dir_;
  bool output_otp_;
  std::string stub_export_include_;
  std::string stub_export_macro_;
};

#endif /* TAO_IC_BE_GLOBAL_H */
