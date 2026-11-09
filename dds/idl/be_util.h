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

#ifndef OPENDDS_IDL_BE_UTIL_H
#define OPENDDS_IDL_BE_UTIL_H

#include <idl_defines.h>

#include <string>

class AST_Generator;
class AST_Decl;

namespace be_util {

  /// Special BE arg call factored out of DRV_args.
#ifdef TAO_IDL_HAS_STD_STRING_API
  void prep_be_arg(std::string const& s);
#else
  void prep_be_arg(char* s);
#endif

  /// Checks made after parsing args.
  void arg_post_proc();

  /// Display usage of BE-specific options.
  void usage();

  /// Create an AST node generator.
  AST_Generator* generator_init();

  /// Get DDS_ROOT. It is a fatal error if it wasn't set.
  const char* dds_root();

  /// Report a miscellaneous error and abort.
  void misc_error_and_abort(const std::string& message, AST_Decl* node = 0);
}

#endif // if !defined
