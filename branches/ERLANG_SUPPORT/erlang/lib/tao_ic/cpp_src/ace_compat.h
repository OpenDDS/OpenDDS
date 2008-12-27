/*
 * $Id$
 */

#ifndef TAO_IC_ACE_COMPAT_H
#define TAO_IC_ACE_COMPAT_H

#ifndef ACE_LACKS_PRAGMA_ONCE
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Version.h"

#if ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION < 5
# include "idl_bool.h"
# define BE_PI_CONST const
#else
  typedef bool idl_bool;
# define I_TRUE true
# define I_FALSE false
# define BE_PI_CONST 
#endif

#endif /* TAO_IC_ACE_COMPAT_H */
