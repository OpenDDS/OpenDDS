
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl TestUtils
// ------------------------------
#ifndef TESTUTILS_EXPORT_H
#define TESTUTILS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TESTUTILS_HAS_DLL)
#  define TESTUTILS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TESTUTILS_HAS_DLL */

#if !defined (TESTUTILS_HAS_DLL)
#  define TESTUTILS_HAS_DLL 1
#endif /* ! TESTUTILS_HAS_DLL */

#if defined (TESTUTILS_HAS_DLL) && (TESTUTILS_HAS_DLL == 1)
#  if defined (TESTUTILS_BUILD_DLL)
#    define TestUtils_Export ACE_Proper_Export_Flag
#    define TESTUTILS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TESTUTILS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TESTUTILS_BUILD_DLL */
#    define TestUtils_Export ACE_Proper_Import_Flag
#    define TESTUTILS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TESTUTILS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TESTUTILS_BUILD_DLL */
#else /* TESTUTILS_HAS_DLL == 1 */
#  define TestUtils_Export
#  define TESTUTILS_SINGLETON_DECLARATION(T)
#  define TESTUTILS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TESTUTILS_HAS_DLL == 1 */

// Set TESTUTILS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TESTUTILS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TESTUTILS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TESTUTILS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TESTUTILS_NTRACE */

#if (TESTUTILS_NTRACE == 1)
#  define TESTUTILS_TRACE(X)
#else /* (TESTUTILS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TESTUTILS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TESTUTILS_NTRACE == 1) */

#endif /* TESTUTILS_EXPORT_H */

// End of auto generated file.
