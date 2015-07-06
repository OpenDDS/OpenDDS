
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SimpleLatency
// ------------------------------
#ifndef SIMPLELATENCY_EXPORT_H
#define SIMPLELATENCY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SIMPLELATENCY_HAS_DLL)
#  define SIMPLELATENCY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SIMPLELATENCY_HAS_DLL */

#if !defined (SIMPLELATENCY_HAS_DLL)
#  define SIMPLELATENCY_HAS_DLL 1
#endif /* ! SIMPLELATENCY_HAS_DLL */

#if defined (SIMPLELATENCY_HAS_DLL) && (SIMPLELATENCY_HAS_DLL == 1)
#  if defined (SIMPLELATENCY_BUILD_DLL)
#    define SimpleLatency_Export ACE_Proper_Export_Flag
#    define SIMPLELATENCY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SIMPLELATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SIMPLELATENCY_BUILD_DLL */
#    define SimpleLatency_Export ACE_Proper_Import_Flag
#    define SIMPLELATENCY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SIMPLELATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SIMPLELATENCY_BUILD_DLL */
#else /* SIMPLELATENCY_HAS_DLL == 1 */
#  define SimpleLatency_Export
#  define SIMPLELATENCY_SINGLETON_DECLARATION(T)
#  define SIMPLELATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SIMPLELATENCY_HAS_DLL == 1 */

// Set SIMPLELATENCY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SIMPLELATENCY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SIMPLELATENCY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SIMPLELATENCY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SIMPLELATENCY_NTRACE */

#if (SIMPLELATENCY_NTRACE == 1)
#  define SIMPLELATENCY_TRACE(X)
#else /* (SIMPLELATENCY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SIMPLELATENCY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SIMPLELATENCY_NTRACE == 1) */

#endif /* SIMPLELATENCY_EXPORT_H */

// End of auto generated file.
