
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl FaceMessenger
// ------------------------------
#ifndef FACEMESSENGER_EXPORT_H
#define FACEMESSENGER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (FACEMESSENGER_HAS_DLL)
#  define FACEMESSENGER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && FACEMESSENGER_HAS_DLL */

#if !defined (FACEMESSENGER_HAS_DLL)
#  define FACEMESSENGER_HAS_DLL 1
#endif /* ! FACEMESSENGER_HAS_DLL */

#if defined (FACEMESSENGER_HAS_DLL) && (FACEMESSENGER_HAS_DLL == 1)
#  if defined (FACEMESSENGER_BUILD_DLL)
#    define FaceMessenger_Export ACE_Proper_Export_Flag
#    define FACEMESSENGER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define FACEMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* FACEMESSENGER_BUILD_DLL */
#    define FaceMessenger_Export ACE_Proper_Import_Flag
#    define FACEMESSENGER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define FACEMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* FACEMESSENGER_BUILD_DLL */
#else /* FACEMESSENGER_HAS_DLL == 1 */
#  define FaceMessenger_Export
#  define FACEMESSENGER_SINGLETON_DECLARATION(T)
#  define FACEMESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* FACEMESSENGER_HAS_DLL == 1 */

// Set FACEMESSENGER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (FACEMESSENGER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define FACEMESSENGER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define FACEMESSENGER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !FACEMESSENGER_NTRACE */

#if (FACEMESSENGER_NTRACE == 1)
#  define FACEMESSENGER_TRACE(X)
#else /* (FACEMESSENGER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define FACEMESSENGER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (FACEMESSENGER_NTRACE == 1) */

#endif /* FACEMESSENGER_EXPORT_H */

// End of auto generated file.
