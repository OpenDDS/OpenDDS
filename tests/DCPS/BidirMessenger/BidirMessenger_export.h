
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl BidirMessenger
// ------------------------------
#ifndef BIDIRMESSENGER_EXPORT_H
#define BIDIRMESSENGER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (BIDIRMESSENGER_HAS_DLL)
#  define BIDIRMESSENGER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && BIDIRMESSENGER_HAS_DLL */

#if !defined (BIDIRMESSENGER_HAS_DLL)
#  define BIDIRMESSENGER_HAS_DLL 1
#endif /* ! BIDIRMESSENGER_HAS_DLL */

#if defined (BIDIRMESSENGER_HAS_DLL) && (BIDIRMESSENGER_HAS_DLL == 1)
#  if defined (BIDIRMESSENGER_BUILD_DLL)
#    define BidirMessenger_Export ACE_Proper_Export_Flag
#    define BIDIRMESSENGER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define BIDIRMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* BIDIRMESSENGER_BUILD_DLL */
#    define BidirMessenger_Export ACE_Proper_Import_Flag
#    define BIDIRMESSENGER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define BIDIRMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* BIDIRMESSENGER_BUILD_DLL */
#else /* BIDIRMESSENGER_HAS_DLL == 1 */
#  define BidirMessenger_Export
#  define BIDIRMESSENGER_SINGLETON_DECLARATION(T)
#  define BIDIRMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* BIDIRMESSENGER_HAS_DLL == 1 */

// Set BIDIRMESSENGER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (BIDIRMESSENGER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define BIDIRMESSENGER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define BIDIRMESSENGER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !BIDIRMESSENGER_NTRACE */

#if (BIDIRMESSENGER_NTRACE == 1)
#  define BIDIRMESSENGER_TRACE(X)
#else /* (BIDIRMESSENGER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define BIDIRMESSENGER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (BIDIRMESSENGER_NTRACE == 1) */

#endif /* BIDIRMESSENGER_EXPORT_H */

// End of auto generated file.
