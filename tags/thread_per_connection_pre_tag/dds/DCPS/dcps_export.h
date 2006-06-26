
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl TAO_DdsDcps
// ------------------------------
#ifndef TAO_DDSDCPS_EXPORT_H
#define TAO_DDSDCPS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)

# if !defined (TAO_DDSDCPS_HAS_DLL)
#   define TAO_DDSDCPS_HAS_DLL 0
# endif /* ! TAO_DDSDCPS_HAS_DLL */
#else
# if !defined (TAO_DDSDCPS_HAS_DLL)
#   define TAO_DDSDCPS_HAS_DLL 1
# endif /* ! TAO_DDSDCPS_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */


#if defined (TAO_DDSDCPS_HAS_DLL) && (TAO_DDSDCPS_HAS_DLL == 1)
#  if defined (TAO_DDSDCPS_BUILD_DLL)
#    define TAO_DdsDcps_Export ACE_Proper_Export_Flag
#    define TAO_DDSDCPS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TAO_DDSDCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TAO_DDSDCPS_BUILD_DLL */
#    define TAO_DdsDcps_Export ACE_Proper_Import_Flag
#    define TAO_DDSDCPS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TAO_DDSDCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TAO_DDSDCPS_BUILD_DLL */
#else /* TAO_DDSDCPS_HAS_DLL == 1 */
#  define TAO_DdsDcps_Export
#  define TAO_DDSDCPS_SINGLETON_DECLARATION(T)
#  define TAO_DDSDCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TAO_DDSDCPS_HAS_DLL == 1 */

// Set TAO_DDSDCPS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TAO_DDSDCPS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TAO_DDSDCPS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TAO_DDSDCPS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TAO_DDSDCPS_NTRACE */

#if (TAO_DDSDCPS_NTRACE == 1)
#  define TAO_DDSDCPS_TRACE(X)
#else /* (TAO_DDSDCPS_NTRACE == 1) */
#  define TAO_DDSDCPS_TRACE(X) ACE_TRACE_IMPL(X)
#endif /* (TAO_DDSDCPS_NTRACE == 1) */

#endif /* TAO_DDSDCPS_EXPORT_H */

// End of auto generated file.
