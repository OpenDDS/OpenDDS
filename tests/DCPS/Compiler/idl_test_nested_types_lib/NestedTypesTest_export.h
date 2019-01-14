
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl NestedTypesTest
// ------------------------------
#ifndef NESTEDTYPESTEST_EXPORT_H
#define NESTEDTYPESTEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (NESTEDTYPESTEST_HAS_DLL)
#  define NESTEDTYPESTEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && NESTEDTYPESTEST_HAS_DLL */

#if !defined (NESTEDTYPESTEST_HAS_DLL)
#  define NESTEDTYPESTEST_HAS_DLL 1
#endif /* ! NESTEDTYPESTEST_HAS_DLL */

#if defined (NESTEDTYPESTEST_HAS_DLL) && (NESTEDTYPESTEST_HAS_DLL == 1)
#  if defined (NESTEDTYPESTEST_BUILD_DLL)
#    define NestedTypesTest_Export ACE_Proper_Export_Flag
#    define NESTEDTYPESTEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define NESTEDTYPESTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* NESTEDTYPESTEST_BUILD_DLL */
#    define NestedTypesTest_Export ACE_Proper_Import_Flag
#    define NESTEDTYPESTEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define NESTEDTYPESTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* NESTEDTYPESTEST_BUILD_DLL */
#else /* NESTEDTYPESTEST_HAS_DLL == 1 */
#  define NestedTypesTest_Export
#  define NESTEDTYPESTEST_SINGLETON_DECLARATION(T)
#  define NESTEDTYPESTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* NESTEDTYPESTEST_HAS_DLL == 1 */

// Set NESTEDTYPESTEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (NESTEDTYPESTEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define NESTEDTYPESTEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define NESTEDTYPESTEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !NESTEDTYPESTEST_NTRACE */

#if (NESTEDTYPESTEST_NTRACE == 1)
#  define NESTEDTYPESTEST_TRACE(X)
#else /* (NESTEDTYPESTEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define NESTEDTYPESTEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (NESTEDTYPESTEST_NTRACE == 1) */

#endif /* NESTEDTYPESTEST_EXPORT_H */

// End of auto generated file.
