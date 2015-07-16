
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Priority
// ------------------------------
#ifndef PRIORITY_EXPORT_H
#define PRIORITY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (PRIORITY_HAS_DLL)
#  define PRIORITY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && PRIORITY_HAS_DLL */

#if !defined (PRIORITY_HAS_DLL)
#  define PRIORITY_HAS_DLL 1
#endif /* ! PRIORITY_HAS_DLL */

#if defined (PRIORITY_HAS_DLL) && (PRIORITY_HAS_DLL == 1)
#  if defined (PRIORITY_BUILD_DLL)
#    define Priority_Export ACE_Proper_Export_Flag
#    define PRIORITY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define PRIORITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* PRIORITY_BUILD_DLL */
#    define Priority_Export ACE_Proper_Import_Flag
#    define PRIORITY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define PRIORITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* PRIORITY_BUILD_DLL */
#else /* PRIORITY_HAS_DLL == 1 */
#  define Priority_Export
#  define PRIORITY_SINGLETON_DECLARATION(T)
#  define PRIORITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* PRIORITY_HAS_DLL == 1 */

// Set PRIORITY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (PRIORITY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define PRIORITY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define PRIORITY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !PRIORITY_NTRACE */

#if (PRIORITY_NTRACE == 1)
#  define PRIORITY_TRACE(X)
#else /* (PRIORITY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define PRIORITY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (PRIORITY_NTRACE == 1) */

#endif /* PRIORITY_EXPORT_H */

// End of auto generated file.
