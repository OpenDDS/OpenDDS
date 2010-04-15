/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "../Version.h"

#include "global_extern.h"
#include "be_extern.h"

void
BE_version()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("OpenDDS version ") ACE_TEXT(DDS_VERSION)
             ACE_TEXT("\n")));
}

int
BE_init(int&, ACE_TCHAR*[])
{
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
  return 0;
}

void
BE_post_init(char*[], long)
{
}
