
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl LargeSample
// ------------------------------
#ifndef LARGESAMPLE_EXPORT_H
#define LARGESAMPLE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (LARGESAMPLE_HAS_DLL)
#  define LARGESAMPLE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && LARGESAMPLE_HAS_DLL */

#if !defined (LARGESAMPLE_HAS_DLL)
#  define LARGESAMPLE_HAS_DLL 1
#endif /* ! LARGESAMPLE_HAS_DLL */

#if defined (LARGESAMPLE_HAS_DLL) && (LARGESAMPLE_HAS_DLL == 1)
#  if defined (LARGESAMPLE_BUILD_DLL)
#    define LargeSample_Export ACE_Proper_Export_Flag
#    define LARGESAMPLE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define LARGESAMPLE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* LARGESAMPLE_BUILD_DLL */
#    define LargeSample_Export ACE_Proper_Import_Flag
#    define LARGESAMPLE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define LARGESAMPLE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* LARGESAMPLE_BUILD_DLL */
#else /* LARGESAMPLE_HAS_DLL == 1 */
#  define LargeSample_Export
#  define LARGESAMPLE_SINGLETON_DECLARATION(T)
#  define LARGESAMPLE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* LARGESAMPLE_HAS_DLL == 1 */

// Set LARGESAMPLE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (LARGESAMPLE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define LARGESAMPLE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define LARGESAMPLE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !LARGESAMPLE_NTRACE */

#if (LARGESAMPLE_NTRACE == 1)
#  define LARGESAMPLE_TRACE(X)
#else /* (LARGESAMPLE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define LARGESAMPLE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (LARGESAMPLE_NTRACE == 1) */

#endif /* LARGESAMPLE_EXPORT_H */

// End of auto generated file.
