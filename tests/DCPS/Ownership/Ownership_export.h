
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Ownership
// ------------------------------
#ifndef OWNERSHIP_EXPORT_H
#define OWNERSHIP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OWNERSHIP_HAS_DLL)
#  define OWNERSHIP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OWNERSHIP_HAS_DLL */

#if !defined (OWNERSHIP_HAS_DLL)
#  define OWNERSHIP_HAS_DLL 1
#endif /* ! OWNERSHIP_HAS_DLL */

#if defined (OWNERSHIP_HAS_DLL) && (OWNERSHIP_HAS_DLL == 1)
#  if defined (OWNERSHIP_BUILD_DLL)
#    define Ownership_Export ACE_Proper_Export_Flag
#    define OWNERSHIP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OWNERSHIP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OWNERSHIP_BUILD_DLL */
#    define Ownership_Export ACE_Proper_Import_Flag
#    define OWNERSHIP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OWNERSHIP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OWNERSHIP_BUILD_DLL */
#else /* OWNERSHIP_HAS_DLL == 1 */
#  define Ownership_Export
#  define OWNERSHIP_SINGLETON_DECLARATION(T)
#  define OWNERSHIP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OWNERSHIP_HAS_DLL == 1 */

// Set OWNERSHIP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OWNERSHIP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OWNERSHIP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OWNERSHIP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OWNERSHIP_NTRACE */

#if (OWNERSHIP_NTRACE == 1)
#  define OWNERSHIP_TRACE(X)
#else /* (OWNERSHIP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OWNERSHIP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OWNERSHIP_NTRACE == 1) */

#endif /* OWNERSHIP_EXPORT_H */

// End of auto generated file.
