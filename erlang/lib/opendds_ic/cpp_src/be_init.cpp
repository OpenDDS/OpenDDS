/*
 * $Id$
 */

#include "ace/Log_Msg.h"

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
  ACE_DEBUG((LM_DEBUG,
             "OPENDDS_IC_BE, version %s\n",
             ACE_TEXT(DDS_VERSION)));
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
