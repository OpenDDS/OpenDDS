/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    be_util.h
 *
 *
 *  Static helper methods used by multiple visitors.
 *
 *
 *  @author Gary Maxey
 *  @author Jeff Parsons
 */
//=============================================================================

#ifndef IDL2JNI_BE_UTIL_H
#define IDL2JNI_BE_UTIL_H

#include <idl_defines.h>

#include <string>

class TAO_OutStream;
class be_module;
class AST_Decl;
class AST_Generator;

class be_util
{
public:
  /// Special BE arg call factored out of DRV_args.
  static void
#ifdef TAO_IDL_HAS_STD_STRING_API
  prep_be_arg (const std::string& s);
#else
  prep_be_arg (char *s);
#endif

  /// Checks made after parsing args.
  static void
  arg_post_proc (void);

  /// Display usage of BE-specific options.
  static void
  usage (void);

  /// Create an AST node generator.
  static AST_Generator *
  generator_init (void);
};

#endif // if !defined
