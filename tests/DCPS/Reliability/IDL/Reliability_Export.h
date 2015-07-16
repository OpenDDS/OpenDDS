
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Reliability
// ------------------------------
#ifndef RELIABILITY_EXPORT_H
#define RELIABILITY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (RELIABILITY_HAS_DLL)
#  define RELIABILITY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && RELIABILITY_HAS_DLL */

#if !defined (RELIABILITY_HAS_DLL)
#  define RELIABILITY_HAS_DLL 1
#endif /* ! RELIABILITY_HAS_DLL */

#if defined (RELIABILITY_HAS_DLL) && (RELIABILITY_HAS_DLL == 1)
#  if defined (RELIABILITY_BUILD_DLL)
#    define Reliability_Export ACE_Proper_Export_Flag
#    define RELIABILITY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define RELIABILITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* RELIABILITY_BUILD_DLL */
#    define Reliability_Export ACE_Proper_Import_Flag
#    define RELIABILITY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define RELIABILITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* RELIABILITY_BUILD_DLL */
#else /* RELIABILITY_HAS_DLL == 1 */
#  define Reliability_Export
#  define RELIABILITY_SINGLETON_DECLARATION(T)
#  define RELIABILITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* RELIABILITY_HAS_DLL == 1 */

// Set RELIABILITY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (RELIABILITY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define RELIABILITY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define RELIABILITY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !RELIABILITY_NTRACE */

#if (RELIABILITY_NTRACE == 1)
#  define RELIABILITY_TRACE(X)
#else /* (RELIABILITY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define RELIABILITY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (RELIABILITY_NTRACE == 1) */

#endif /* RELIABILITY_EXPORT_H */

// End of auto generated file.
