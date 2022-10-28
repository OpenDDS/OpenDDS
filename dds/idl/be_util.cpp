
//=============================================================================
/**
 *  @file    be_util.cpp
 *
 *
 *  Static helper methods used by multiple visitors.
 *
 *
 *  @author Gary Maxey
 *  @author Jeff Parsons
 */
//=============================================================================

#include "be_util.h"

#include "be_init.h"
#include "be_extern.h"

#include <ast_generator.h>

// Prepare an argument for a BE
void
be_util::prep_be_arg(char* arg)
{
  BE_prep_be_arg(arg);
}

void
be_util::arg_post_proc()
{
  BE_arg_post_proc();
}

void
be_util::usage()
{
  BE_usage();
}

AST_Generator*
be_util::generator_init()
{
  AST_Generator* gen = 0;
  ACE_NEW_RETURN(gen, AST_Generator, 0);
  return gen;
}

const char*
be_util::dds_root()
{
  static const char* value = ACE_OS::getenv("DDS_ROOT");
  if (!value || !value[0]) {
    ACE_ERROR((LM_ERROR, "Error - The environment variable DDS_ROOT must be set.\n"));
    BE_abort();
  }
  return value;
}

void
be_util::misc_error_and_abort(const std::string& message, AST_Decl* node)
{
  idl_global->err()->misc_error(message.c_str(), node);
  BE_abort();
}
