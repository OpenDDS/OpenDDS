
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl KeyTest
// ------------------------------
#ifndef KEYTEST_EXPORT_H
#define KEYTEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (KEYTEST_HAS_DLL)
#  define KEYTEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && KEYTEST_HAS_DLL */

#if !defined (KEYTEST_HAS_DLL)
#  define KEYTEST_HAS_DLL 1
#endif /* ! KEYTEST_HAS_DLL */

#if defined (KEYTEST_HAS_DLL) && (KEYTEST_HAS_DLL == 1)
#  if defined (KEYTEST_BUILD_DLL)
#    define KeyTest_Export ACE_Proper_Export_Flag
#    define KEYTEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define KEYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* KEYTEST_BUILD_DLL */
#    define KeyTest_Export ACE_Proper_Import_Flag
#    define KEYTEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define KEYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* KEYTEST_BUILD_DLL */
#else /* KEYTEST_HAS_DLL == 1 */
#  define KeyTest_Export
#  define KEYTEST_SINGLETON_DECLARATION(T)
#  define KEYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* KEYTEST_HAS_DLL == 1 */

// Set KEYTEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (KEYTEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define KEYTEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define KEYTEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !KEYTEST_NTRACE */

#if (KEYTEST_NTRACE == 1)
#  define KEYTEST_TRACE(X)
#else /* (KEYTEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define KEYTEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (KEYTEST_NTRACE == 1) */

#endif /* KEYTEST_EXPORT_H */

// End of auto generated file.
