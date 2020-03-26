/*
 * Common Definitions for Windows Resource Compiler (rc) Files in OpenDDS
 */

#ifndef OPENDDS_RC_COMMON_HEADER
#define OPENDDS_RC_COMMON_HEADER

#include "Version.h"

#include <winver.h>

#if OPENDDS_IS_RELEASE
#  define OPENDDS_RC_VS_FF_PRERELEASE 0x00L
#else
#  define OPENDDS_RC_VS_FF_PRERELEASE VS_FF_PRERELEASE
#endif

#define OPENDDS_RC_FILEFLAGS (OPENDDS_RC_VS_FF_PRERELEASE)

#endif
