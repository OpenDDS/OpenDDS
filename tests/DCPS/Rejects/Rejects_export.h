
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Rejects
// ------------------------------
#ifndef REJECTS_EXPORT_H
#define REJECTS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (REJECTS_HAS_DLL)
#  define REJECTS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && REJECTS_HAS_DLL */

#if !defined (REJECTS_HAS_DLL)
#  define REJECTS_HAS_DLL 1
#endif /* ! REJECTS_HAS_DLL */

#if defined (REJECTS_HAS_DLL) && (REJECTS_HAS_DLL == 1)
#  if defined (REJECTS_BUILD_DLL)
#    define Rejects_Export ACE_Proper_Export_Flag
#    define REJECTS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define REJECTS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* REJECTS_BUILD_DLL */
#    define Rejects_Export ACE_Proper_Import_Flag
#    define REJECTS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define REJECTS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* REJECTS_BUILD_DLL */
#else /* REJECTS_HAS_DLL == 1 */
#  define Rejects_Export
#  define REJECTS_SINGLETON_DECLARATION(T)
#  define REJECTS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* REJECTS_HAS_DLL == 1 */

// Set REJECTS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (REJECTS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define REJECTS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define REJECTS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !REJECTS_NTRACE */

#if (REJECTS_NTRACE == 1)
#  define REJECTS_TRACE(X)
#else /* (REJECTS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define REJECTS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (REJECTS_NTRACE == 1) */

#endif /* REJECTS_EXPORT_H */

// End of auto generated file.
