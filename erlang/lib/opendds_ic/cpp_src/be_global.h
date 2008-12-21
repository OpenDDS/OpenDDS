/*
 * $Id$
 */

#ifndef OPENDDS_IC_BE_GLOBAL_H
#define OPENDDS_IC_BE_GLOBAL_H

#ifndef ACE_LACKS_PRAGMA_ONCE
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class AST_Generator;

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

  /// BE specific members
};

#endif /* OPENDDS_IC_BE_GLOBAL_H */
