
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s Sync
// ------------------------------
#ifndef SYNC_EXPORT_H
#define SYNC_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SYNC_HAS_DLL)
#  define SYNC_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SYNC_HAS_DLL */

#if !defined (SYNC_HAS_DLL)
#  define SYNC_HAS_DLL 1
#endif /* ! SYNC_HAS_DLL */

#if defined (SYNC_HAS_DLL) && (SYNC_HAS_DLL == 1)
#  if defined (SYNC_BUILD_DLL)
#    define Sync_Export ACE_Proper_Export_Flag
#    define SYNC_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SYNC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SYNC_BUILD_DLL */
#    define Sync_Export ACE_Proper_Import_Flag
#    define SYNC_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SYNC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SYNC_BUILD_DLL */
#else /* SYNC_HAS_DLL == 1 */
#  define Sync_Export
#  define SYNC_SINGLETON_DECLARATION(T)
#  define SYNC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SYNC_HAS_DLL == 1 */

// Set SYNC_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SYNC_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SYNC_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SYNC_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SYNC_NTRACE */

#if (SYNC_NTRACE == 1)
#  define SYNC_TRACE(X)
#else /* (SYNC_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SYNC_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SYNC_NTRACE == 1) */

#endif /* SYNC_EXPORT_H */

// End of auto generated file.
