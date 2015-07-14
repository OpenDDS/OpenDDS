
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl TestFramework
// ------------------------------
#ifndef TESTFRAMEWORK_EXPORT_H
#define TESTFRAMEWORK_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TESTFRAMEWORK_HAS_DLL)
#  define TESTFRAMEWORK_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TESTFRAMEWORK_HAS_DLL */

#if !defined (TESTFRAMEWORK_HAS_DLL)
#  define TESTFRAMEWORK_HAS_DLL 1
#endif /* ! TESTFRAMEWORK_HAS_DLL */

#if defined (TESTFRAMEWORK_HAS_DLL) && (TESTFRAMEWORK_HAS_DLL == 1)
#  if defined (TESTFRAMEWORK_BUILD_DLL)
#    define TestFramework_Export ACE_Proper_Export_Flag
#    define TESTFRAMEWORK_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TESTFRAMEWORK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TESTFRAMEWORK_BUILD_DLL */
#    define TestFramework_Export ACE_Proper_Import_Flag
#    define TESTFRAMEWORK_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TESTFRAMEWORK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TESTFRAMEWORK_BUILD_DLL */
#else /* TESTFRAMEWORK_HAS_DLL == 1 */
#  define TestFramework_Export
#  define TESTFRAMEWORK_SINGLETON_DECLARATION(T)
#  define TESTFRAMEWORK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TESTFRAMEWORK_HAS_DLL == 1 */

// Set TESTFRAMEWORK_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TESTFRAMEWORK_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TESTFRAMEWORK_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TESTFRAMEWORK_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TESTFRAMEWORK_NTRACE */

#if (TESTFRAMEWORK_NTRACE == 1)
#  define TESTFRAMEWORK_TRACE(X)
#else /* (TESTFRAMEWORK_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TESTFRAMEWORK_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TESTFRAMEWORK_NTRACE == 1) */

#endif /* TESTFRAMEWORK_EXPORT_H */

// End of auto generated file.
