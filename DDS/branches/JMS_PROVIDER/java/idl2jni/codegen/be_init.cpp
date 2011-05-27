

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
BE_init (int &, char *[])
{
#if ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION >= 5
  ACE_NEW_RETURN (be_global, BE_GlobalData, -1);
#endif
  return 0;
}

void
BE_post_init (BE_PI_CONST char *[], long)
{
}

