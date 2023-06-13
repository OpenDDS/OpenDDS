#include <ace/config.h>
#include <ace/Version.h>

#include <stdio.h>

int main(int argc, char**)
{
  // Output value to match to https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
  // As of writing these values aren't documented, see
  // https://gitlab.kitware.com/cmake/cmake/-/issues/21489
  printf("CMAKE_SYSTEM_NAME=%s\n",
#if defined ACE_LINUX
    "Linux"
#elif defined ACE_WIN32
    "Windows"
#elif defined ACE_HAS_MAC_OSX
    "Darwin"
#else
    "unknown"
#endif
  );

  printf("ACE_VERSION=%s\n", ACE_VERSION);

  return 0;
}
