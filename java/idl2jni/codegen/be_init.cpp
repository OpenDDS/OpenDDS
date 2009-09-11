

#include "global_extern.h"
#include "be_extern.h"
#include "tao/Version.h"


void
BE_version (void)
{
  ACE_DEBUG ((LM_DEBUG,
              "%s %s\n",
              ACE_TEXT ("IDL2JNI_BE, version"),
              ACE_TEXT (TAO_VERSION)));
}

int
BE_init (int &, ACE_TCHAR *[])
{
  ACE_NEW_RETURN (be_global, BE_GlobalData, -1);
  return 0;
}

void
BE_post_init (char *[], long)
{
}

