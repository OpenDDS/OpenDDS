/*
 * $Id$
 */

#include <iostream>

#include "dds/Version.h"

#include "be_extern.h"

int
BE_init(int &, char **)
{
  return 0;
}

void
BE_post_init(BE_PI_CONST char **, long)
{
}

void
BE_version()
{
  std::cerr << "OPENDDS_IC_BE, version " << DDS_VERSION << std::endl;
}

void
BE_produce()
{
}

void
BE_abort()
{
}

void
BE_cleanup()
{
}
