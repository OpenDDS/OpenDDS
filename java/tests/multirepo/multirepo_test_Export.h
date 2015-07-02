// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl multirepo_test
// ------------------------------
#ifndef MULTIREPO_TEST_EXPORT_H
#define MULTIREPO_TEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (MULTIREPO_TEST_HAS_DLL)
#  define MULTIREPO_TEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && MULTIREPO_TEST_HAS_DLL */

#if !defined (MULTIREPO_TEST_HAS_DLL)
#  define MULTIREPO_TEST_HAS_DLL 1
#endif /* !MULTIREPO_TEST_HAS_DLL */

#if defined (MULTIREPO_TEST_HAS_DLL) && (MULTIREPO_TEST_HAS_DLL == 1)
#  if defined (MULTIREPO_TEST_BUILD_DLL)
#    define multirepo_test_Export ACE_Proper_Export_Flag
#    define MULTIREPO_TEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define MULTIREPO_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* MULTIREPO_TEST_BUILD_DLL */
#    define multirepo_test_Export ACE_Proper_Import_Flag
#    define MULTIREPO_TEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define MULTIREPO_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* MULTIREPO_TEST_BUILD_DLL */
#else /* MULTIREPO_TEST_HAS_DLL == 1 */
#  define multirepo_test_Export
#  define MULTIREPO_TEST_SINGLETON_DECLARATION(T)
#  define MULTIREPO_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* MULTIREPO_TEST_HAS_DLL == 1 */

// Set MULTIREPO_TEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (MULTIREPO_TEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define MULTIREPO_TEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define MULTIREPO_TEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !MULTIREPO_TEST_NTRACE */

#if (MULTIREPO_TEST_NTRACE == 1)
#  define MULTIREPO_TEST_TRACE(X)
#else /* (MULTIREPO_TEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define MULTIREPO_TEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (MULTIREPO_TEST_NTRACE == 1) */

#endif /* MULTIREPO_TEST_EXPORT_H */

// End of auto generated file.
