/*
 * $Id$
 */

#include <iostream>
#include <new>

#include "ace/Log_Msg.h"
#include "dds/Version.h"

#include "be_extern.h"
#include "global_extern.h"

int
BE_init(int &, char **)
{
  be_global = new(std::nothrow) BE_GlobalData;
  if (be_global == 0) {
    return -1;
  }

  idl_global->preserve_cpp_keywords(I_TRUE);
  return 0;
}

void
BE_post_init(BE_PI_CONST char **, long)
{
}

void
BE_version()
{
  std::cerr 
    << "OPENDDS_IC_BE, version " << DDS_VERSION
    << std::endl;
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
