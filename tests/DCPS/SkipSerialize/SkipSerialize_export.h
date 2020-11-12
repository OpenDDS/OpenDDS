
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SkipSerialize
// ------------------------------
#ifndef SKIPSERIALIZE_EXPORT_H
#define SKIPSERIALIZE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SKIPSERIALIZE_HAS_DLL)
#  define SKIPSERIALIZE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SKIPSERIALIZE_HAS_DLL */

#if !defined (SKIPSERIALIZE_HAS_DLL)
#  define SKIPSERIALIZE_HAS_DLL 1
#endif /* ! SKIPSERIALIZE_HAS_DLL */

#if defined (SKIPSERIALIZE_HAS_DLL) && (SKIPSERIALIZE_HAS_DLL == 1)
#  if defined (SKIPSERIALIZE_BUILD_DLL)
#    define SkipSerialize_Export ACE_Proper_Export_Flag
#    define SKIPSERIALIZE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SKIPSERIALIZE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SKIPSERIALIZE_BUILD_DLL */
#    define SkipSerialize_Export ACE_Proper_Import_Flag
#    define SKIPSERIALIZE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SKIPSERIALIZE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SKIPSERIALIZE_BUILD_DLL */
#else /* SKIPSERIALIZE_HAS_DLL == 1 */
#  define SkipSerialize_Export
#  define SKIPSERIALIZE_SINGLETON_DECLARATION(T)
#  define SKIPSERIALIZE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SKIPSERIALIZE_HAS_DLL == 1 */

// Set SKIPSERIALIZE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SKIPSERIALIZE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SKIPSERIALIZE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SKIPSERIALIZE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SKIPSERIALIZE_NTRACE */

#if (SKIPSERIALIZE_NTRACE == 1)
#  define SKIPSERIALIZE_TRACE(X)
#else /* (SKIPSERIALIZE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SKIPSERIALIZE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SKIPSERIALIZE_NTRACE == 1) */

#endif /* SKIPSERIALIZE_EXPORT_H */

// End of auto generated file.
