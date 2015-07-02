
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Deadline
// ------------------------------
#ifndef DEADLINE_EXPORT_H
#define DEADLINE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DEADLINE_HAS_DLL)
#  define DEADLINE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DEADLINE_HAS_DLL */

#if !defined (DEADLINE_HAS_DLL)
#  define DEADLINE_HAS_DLL 1
#endif /* ! DEADLINE_HAS_DLL */

#if defined (DEADLINE_HAS_DLL) && (DEADLINE_HAS_DLL == 1)
#  if defined (DEADLINE_BUILD_DLL)
#    define Deadline_Export ACE_Proper_Export_Flag
#    define DEADLINE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DEADLINE_BUILD_DLL */
#    define Deadline_Export ACE_Proper_Import_Flag
#    define DEADLINE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DEADLINE_BUILD_DLL */
#else /* DEADLINE_HAS_DLL == 1 */
#  define Deadline_Export
#  define DEADLINE_SINGLETON_DECLARATION(T)
#  define DEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DEADLINE_HAS_DLL == 1 */

// Set DEADLINE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DEADLINE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DEADLINE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DEADLINE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DEADLINE_NTRACE */

#if (DEADLINE_NTRACE == 1)
#  define DEADLINE_TRACE(X)
#else /* (DEADLINE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DEADLINE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DEADLINE_NTRACE == 1) */

#endif /* DEADLINE_EXPORT_H */

// End of auto generated file.
