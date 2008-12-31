/*
 * $Id$
 */

#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_Memory.h"
#include "ace/Version.h"

#include "tao/Version.h"

#include "ace_compat.h"
#include "be_extern.h"
#include "global_extern.h"

int
BE_init(int &, char **)
{
#ifndef ACE_PRE_5_5
  ACE_NEW_RETURN(be_global, BE_GlobalData, -1);
#endif /* ACE_PRE_5_5 */
  return 0;
}

void
BE_post_init(BE_PI_CONST char **, long)
{
  idl_global->preserve_cpp_keywords(I_TRUE);
}

void
BE_version()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("TAO_IC_BE, version %s (Erlang Port Driver IDL BE)\n"),
             ACE_TEXT(TAO_VERSION)));
}

void
BE_abort()
{
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("TAO_IC_BE: aborting")));

  BE_cleanup();

  ACE_OS::exit(1);
}

void
BE_cleanup()
{
  idl_global->destroy();
}

void
BE_produce()
{
  
}

