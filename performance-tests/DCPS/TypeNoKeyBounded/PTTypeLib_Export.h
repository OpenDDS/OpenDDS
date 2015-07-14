
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl PTTypeLib
// ------------------------------
#ifndef PTTYPELIB_EXPORT_H
#define PTTYPELIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)

# if !defined (PTTYPELIB_HAS_DLL)
#   define PTTYPELIB_HAS_DLL 0
# endif /* ! PTTYPELIB_HAS_DLL */
#else
#if !defined (PTTYPELIB_HAS_DLL)
#  define PTTYPELIB_HAS_DLL 1
#endif /* ! PTTYPELIB_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */

#if defined (PTTYPELIB_HAS_DLL) && (PTTYPELIB_HAS_DLL == 1)
#  if defined (PTTYPELIB_BUILD_DLL)
#    define PTTypeLib_Export ACE_Proper_Export_Flag
#    define PTTYPELIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define PTTYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* PTTYPELIB_BUILD_DLL */
#    define PTTypeLib_Export ACE_Proper_Import_Flag
#    define PTTYPELIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define PTTYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* PTTYPELIB_BUILD_DLL */
#else /* PTTYPELIB_HAS_DLL == 1 */
#  define PTTypeLib_Export
#  define PTTYPELIB_SINGLETON_DECLARATION(T)
#  define PTTYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* PTTYPELIB_HAS_DLL == 1 */

// Set PTTYPELIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (PTTYPELIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define PTTYPELIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define PTTYPELIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !PTTYPELIB_NTRACE */

#if (PTTYPELIB_NTRACE == 1)
#  define PTTYPELIB_TRACE(X)
#else /* (PTTYPELIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define PTTYPELIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (PTTYPELIB_NTRACE == 1) */

#endif /* PTTYPELIB_EXPORT_H */

// End of auto generated file.
