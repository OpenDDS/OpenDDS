
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Messenger
// ------------------------------
#ifndef MESSENGER_EXPORT_H
#define MESSENGER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (MESSENGER_HAS_DLL)
#  define MESSENGER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && MESSENGER_HAS_DLL */

#if !defined (MESSENGER_HAS_DLL)
#  define MESSENGER_HAS_DLL 1
#endif /* ! MESSENGER_HAS_DLL */

#if defined (MESSENGER_HAS_DLL) && (MESSENGER_HAS_DLL == 1)
#  if defined (MESSENGER_BUILD_DLL)
#    define Messenger_Export ACE_Proper_Export_Flag
#    define MESSENGER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define MESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* MESSENGER_BUILD_DLL */
#    define Messenger_Export ACE_Proper_Import_Flag
#    define MESSENGER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define MESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* MESSENGER_BUILD_DLL */
#else /* MESSENGER_HAS_DLL == 1 */
#  define Messenger_Export
#  define MESSENGER_SINGLETON_DECLARATION(T)
#  define MESSENGER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* MESSENGER_HAS_DLL == 1 */

// Set MESSENGER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (MESSENGER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define MESSENGER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define MESSENGER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !MESSENGER_NTRACE */

#if (MESSENGER_NTRACE == 1)
#  define MESSENGER_TRACE(X)
#else /* (MESSENGER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define MESSENGER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (MESSENGER_NTRACE == 1) */

#endif /* MESSENGER_EXPORT_H */

// End of auto generated file.
