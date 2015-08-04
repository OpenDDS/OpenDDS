
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl FaceHeaderTest
// ------------------------------
#ifndef FACEHEADERTEST_EXPORT_H
#define FACEHEADERTEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (FACEHEADERTEST_HAS_DLL)
#  define FACEHEADERTEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && FACEHEADERTEST_HAS_DLL */

#if !defined (FACEHEADERTEST_HAS_DLL)
#  define FACEHEADERTEST_HAS_DLL 1
#endif /* ! FACEHEADERTEST_HAS_DLL */

#if defined (FACEHEADERTEST_HAS_DLL) && (FACEHEADERTEST_HAS_DLL == 1)
#  if defined (FACEHEADERTEST_BUILD_DLL)
#    define FaceHeaderTest_Export ACE_Proper_Export_Flag
#    define FACEHEADERTEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define FACEHEADERTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* FACEHEADERTEST_BUILD_DLL */
#    define FaceHeaderTest_Export ACE_Proper_Import_Flag
#    define FACEHEADERTEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define FACEHEADERTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* FACEHEADERTEST_BUILD_DLL */
#else /* FACEHEADERTEST_HAS_DLL == 1 */
#  define FaceHeaderTest_Export
#  define FACEHEADERTEST_SINGLETON_DECLARATION(T)
#  define FACEHEADERTEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* FACEHEADERTEST_HAS_DLL == 1 */

// Set FACEHEADERTEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (FACEHEADERTEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define FACEHEADERTEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define FACEHEADERTEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !FACEHEADERTEST_NTRACE */

#if (FACEHEADERTEST_NTRACE == 1)
#  define FACEHEADERTEST_TRACE(X)
#else /* (FACEHEADERTEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define FACEHEADERTEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (FACEHEADERTEST_NTRACE == 1) */

#endif /* FACEHEADERTEST_EXPORT_H */

// End of auto generated file.
