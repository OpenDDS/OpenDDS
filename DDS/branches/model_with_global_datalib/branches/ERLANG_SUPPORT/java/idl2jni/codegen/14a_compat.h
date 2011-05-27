/* -*- C++ -*- */

#ifndef IDL2JNI_BE_14A_COMPAT_H
#define IDL2JNI_BE_14A_COMPAT_H

#include "ace/Version.h"

#if ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION < 5
#  include "idl_bool.h"
#  define BE_PI_CONST const
#else
   typedef bool idl_bool;
#  define I_TRUE true
#  define I_FALSE false
#  define BE_PI_CONST 
#endif //ACE version

#endif //header guard
