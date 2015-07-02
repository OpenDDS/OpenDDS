
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Reconnect
// ------------------------------
#ifndef RECONNECT_EXPORT_H
#define RECONNECT_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (RECONNECT_HAS_DLL)
#  define RECONNECT_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && RECONNECT_HAS_DLL */

#if !defined (RECONNECT_HAS_DLL)
#  define RECONNECT_HAS_DLL 1
#endif /* ! RECONNECT_HAS_DLL */

#if defined (RECONNECT_HAS_DLL) && (RECONNECT_HAS_DLL == 1)
#  if defined (RECONNECT_BUILD_DLL)
#    define Reconnect_Export ACE_Proper_Export_Flag
#    define RECONNECT_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define RECONNECT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* RECONNECT_BUILD_DLL */
#    define Reconnect_Export ACE_Proper_Import_Flag
#    define RECONNECT_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define RECONNECT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* RECONNECT_BUILD_DLL */
#else /* RECONNECT_HAS_DLL == 1 */
#  define Reconnect_Export
#  define RECONNECT_SINGLETON_DECLARATION(T)
#  define RECONNECT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* RECONNECT_HAS_DLL == 1 */

// Set RECONNECT_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (RECONNECT_NTRACE)
#  if (ACE_NTRACE == 1)
#    define RECONNECT_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define RECONNECT_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !RECONNECT_NTRACE */

#if (RECONNECT_NTRACE == 1)
#  define RECONNECT_TRACE(X)
#else /* (RECONNECT_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define RECONNECT_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (RECONNECT_NTRACE == 1) */

#endif /* RECONNECT_EXPORT_H */

// End of auto generated file.
