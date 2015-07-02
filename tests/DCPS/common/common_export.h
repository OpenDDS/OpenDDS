
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Common
// ------------------------------
#ifndef COMMON_EXPORT_H
#define COMMON_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)
# if !defined (COMMON_HAS_DLL)
#   define COMMON_HAS_DLL 0
# endif /* ! COMMON_HAS_DLL */
#else
#if !defined (COMMON_HAS_DLL)
#  define COMMON_HAS_DLL 1
#endif /* ! COMMON_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */

#if defined (COMMON_HAS_DLL) && (COMMON_HAS_DLL == 1)
#  if defined (COMMON_BUILD_DLL)
#    define Common_Export ACE_Proper_Export_Flag
#    define COMMON_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define COMMON_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* COMMON_BUILD_DLL */
#    define Common_Export ACE_Proper_Import_Flag
#    define COMMON_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define COMMON_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* COMMON_BUILD_DLL */
#else /* COMMON_HAS_DLL == 1 */
#  define Common_Export
#  define COMMON_SINGLETON_DECLARATION(T)
#  define COMMON_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* COMMON_HAS_DLL == 1 */

// Set COMMON_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (COMMON_NTRACE)
#  if (ACE_NTRACE == 1)
#    define COMMON_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define COMMON_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !COMMON_NTRACE */

#if (COMMON_NTRACE == 1)
#  define COMMON_TRACE(X)
#else /* (COMMON_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define COMMON_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (COMMON_NTRACE == 1) */

#endif /* COMMON_EXPORT_H */

// End of auto generated file.
