
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl NotifyTest
// ------------------------------
#ifndef NOTIFYTEST_EXPORT_H
#define NOTIFYTEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (NOTIFYTEST_HAS_DLL)
#  define NOTIFYTEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && NOTIFYTEST_HAS_DLL */

#if !defined (NOTIFYTEST_HAS_DLL)
#  define NOTIFYTEST_HAS_DLL 1
#endif /* ! NOTIFYTEST_HAS_DLL */

#if defined (NOTIFYTEST_HAS_DLL) && (NOTIFYTEST_HAS_DLL == 1)
#  if defined (NOTIFYTEST_BUILD_DLL)
#    define NotifyTest_Export ACE_Proper_Export_Flag
#    define NOTIFYTEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define NOTIFYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* NOTIFYTEST_BUILD_DLL */
#    define NotifyTest_Export ACE_Proper_Import_Flag
#    define NOTIFYTEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define NOTIFYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* NOTIFYTEST_BUILD_DLL */
#else /* NOTIFYTEST_HAS_DLL == 1 */
#  define NotifyTest_Export
#  define NOTIFYTEST_SINGLETON_DECLARATION(T)
#  define NOTIFYTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* NOTIFYTEST_HAS_DLL == 1 */

// Set NOTIFYTEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (NOTIFYTEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define NOTIFYTEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define NOTIFYTEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !NOTIFYTEST_NTRACE */

#if (NOTIFYTEST_NTRACE == 1)
#  define NOTIFYTEST_TRACE(X)
#else /* (NOTIFYTEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define NOTIFYTEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (NOTIFYTEST_NTRACE == 1) */

#endif /* NOTIFYTEST_EXPORT_H */

// End of auto generated file.
