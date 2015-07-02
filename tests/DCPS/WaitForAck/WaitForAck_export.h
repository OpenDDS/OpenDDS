
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl WaitForAck
// ------------------------------
#ifndef WAITFORACK_EXPORT_H
#define WAITFORACK_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (WAITFORACK_HAS_DLL)
#  define WAITFORACK_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && WAITFORACK_HAS_DLL */

#if !defined (WAITFORACK_HAS_DLL)
#  define WAITFORACK_HAS_DLL 1
#endif /* ! WAITFORACK_HAS_DLL */

#if defined (WAITFORACK_HAS_DLL) && (WAITFORACK_HAS_DLL == 1)
#  if defined (WAITFORACK_BUILD_DLL)
#    define WaitForAck_Export ACE_Proper_Export_Flag
#    define WAITFORACK_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define WAITFORACK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* WAITFORACK_BUILD_DLL */
#    define WaitForAck_Export ACE_Proper_Import_Flag
#    define WAITFORACK_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define WAITFORACK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* WAITFORACK_BUILD_DLL */
#else /* WAITFORACK_HAS_DLL == 1 */
#  define WaitForAck_Export
#  define WAITFORACK_SINGLETON_DECLARATION(T)
#  define WAITFORACK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* WAITFORACK_HAS_DLL == 1 */

// Set WAITFORACK_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (WAITFORACK_NTRACE)
#  if (ACE_NTRACE == 1)
#    define WAITFORACK_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define WAITFORACK_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !WAITFORACK_NTRACE */

#if (WAITFORACK_NTRACE == 1)
#  define WAITFORACK_TRACE(X)
#else /* (WAITFORACK_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define WAITFORACK_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (WAITFORACK_NTRACE == 1) */

#endif /* WAITFORACK_EXPORT_H */

// End of auto generated file.
