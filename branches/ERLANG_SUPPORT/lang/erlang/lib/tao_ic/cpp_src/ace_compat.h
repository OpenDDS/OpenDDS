/*
 * $Id$
 */

#ifndef ACE_COMPAT_H
#define ACE_COMPAT_H

#include "ace/Version.h"

#if ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION < 5
# define ACE_PRE_5_5
#endif

#ifdef ACE_PRE_5_5
# include "idl_bool.h"
# define BE_PI_CONST const
#else
typedef bool idl_bool;
# define I_TRUE true
# define I_FALSE false
# define BE_PI_CONST
#endif /* ACE_PRE_5_5 */

#endif /* ACE_COMPAT_H */
