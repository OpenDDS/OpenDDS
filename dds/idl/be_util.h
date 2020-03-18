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

#include <string>

class AST_Generator;

namespace be_util {

  /// Special BE arg call factored out of DRV_args.
  void prep_be_arg(char* s);

  /// Checks made after parsing args.
  void arg_post_proc();

  /// Display usage of BE-specific options.
  void usage();

  /// Create an AST node generator.
  AST_Generator* generator_init();

  /// Get DDS_ROOT. It is a fatal error if it wasn't set.
  const char* dds_root();
};

#endif // if !defined
