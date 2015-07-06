// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl complex_idl_test
// ------------------------------
#ifndef COMPLEX_IDL_TEST_EXPORT_H
#define COMPLEX_IDL_TEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (COMPLEX_IDL_TEST_HAS_DLL)
#  define COMPLEX_IDL_TEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && COMPLEX_IDL_TEST_HAS_DLL */

#if !defined (COMPLEX_IDL_TEST_HAS_DLL)
#  define COMPLEX_IDL_TEST_HAS_DLL 1
#endif /* !COMPLEX_IDL_TEST_HAS_DLL */

#if defined (COMPLEX_IDL_TEST_HAS_DLL) && (COMPLEX_IDL_TEST_HAS_DLL == 1)
#  if defined (COMPLEX_IDL_TEST_BUILD_DLL)
#    define complex_idl_test_Export ACE_Proper_Export_Flag
#    define COMPLEX_IDL_TEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define COMPLEX_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* COMPLEX_IDL_TEST_BUILD_DLL */
#    define complex_idl_test_Export ACE_Proper_Import_Flag
#    define COMPLEX_IDL_TEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define COMPLEX_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* COMPLEX_IDL_TEST_BUILD_DLL */
#else /* COMPLEX_IDL_TEST_HAS_DLL == 1 */
#  define complex_idl_test_Export
#  define COMPLEX_IDL_TEST_SINGLETON_DECLARATION(T)
#  define COMPLEX_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* COMPLEX_IDL_TEST_HAS_DLL == 1 */

// Set COMPLEX_IDL_TEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (COMPLEX_IDL_TEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define COMPLEX_IDL_TEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define COMPLEX_IDL_TEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !COMPLEX_IDL_TEST_NTRACE */

#if (COMPLEX_IDL_TEST_NTRACE == 1)
#  define COMPLEX_IDL_TEST_TRACE(X)
#else /* (COMPLEX_IDL_TEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define COMPLEX_IDL_TEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (COMPLEX_IDL_TEST_NTRACE == 1) */

#endif /* COMPLEX_IDL_TEST_EXPORT_H */

// End of auto generated file.
