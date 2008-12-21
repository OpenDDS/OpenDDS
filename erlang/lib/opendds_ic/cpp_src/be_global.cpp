/*
 * $Id$
 */

#include "be_global.h"

BE_GlobalData *be_global = 0;

BE_GlobalData::BE_GlobalData()
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
BE_GlobalData::prep_be_arg(char *)
{
}

void
BE_GlobalData::arg_post_proc()
{
}

void
BE_GlobalData::usage() const
{
}

AST_Generator *
BE_GlobalData::generator_init()
{
  return 0;
}
