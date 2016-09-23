// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl two_idl_test
// ------------------------------
#ifndef TWO_IDL_TEST_EXPORT_H
#define TWO_IDL_TEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TWO_IDL_TEST_HAS_DLL)
#  define TWO_IDL_TEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TWO_IDL_TEST_HAS_DLL */

#if !defined (TWO_IDL_TEST_HAS_DLL)
#  define TWO_IDL_TEST_HAS_DLL 1
#endif /* !TWO_IDL_TEST_HAS_DLL */

#if defined (TWO_IDL_TEST_HAS_DLL) && (TWO_IDL_TEST_HAS_DLL == 1)
#  if defined (TWO_IDL_TEST_BUILD_DLL)
#    define two_idl_test_Export ACE_Proper_Export_Flag
#    define TWO_IDL_TEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TWO_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TWO_IDL_TEST_BUILD_DLL */
#    define two_idl_test_Export ACE_Proper_Import_Flag
#    define TWO_IDL_TEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TWO_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TWO_IDL_TEST_BUILD_DLL */
#else /* TWO_IDL_TEST_HAS_DLL == 1 */
#  define two_idl_test_Export
#  define TWO_IDL_TEST_SINGLETON_DECLARATION(T)
#  define TWO_IDL_TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TWO_IDL_TEST_HAS_DLL == 1 */

// Set TWO_IDL_TEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TWO_IDL_TEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TWO_IDL_TEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TWO_IDL_TEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TWO_IDL_TEST_NTRACE */

#if (TWO_IDL_TEST_NTRACE == 1)
#  define TWO_IDL_TEST_TRACE(X)
#else /* (TWO_IDL_TEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TWO_IDL_TEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TWO_IDL_TEST_NTRACE == 1) */

#endif /* TWO_IDL_TEST_EXPORT_H */

// End of auto generated file.
