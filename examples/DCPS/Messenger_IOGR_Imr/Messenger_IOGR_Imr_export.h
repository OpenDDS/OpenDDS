
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Messenger_IOGR_Imr
// ------------------------------
#ifndef MESSENGER_IOGR_IMR_EXPORT_H
#define MESSENGER_IOGR_IMR_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (MESSENGER_IOGR_IMR_HAS_DLL)
#  define MESSENGER_IOGR_IMR_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && MESSENGER_IOGR_IMR_HAS_DLL */

#if !defined (MESSENGER_IOGR_IMR_HAS_DLL)
#  define MESSENGER_IOGR_IMR_HAS_DLL 1
#endif /* ! MESSENGER_IOGR_IMR_HAS_DLL */

#if defined (MESSENGER_IOGR_IMR_HAS_DLL) && (MESSENGER_IOGR_IMR_HAS_DLL == 1)
#  if defined (MESSENGER_IOGR_IMR_BUILD_DLL)
#    define Messenger_IOGR_Imr_Export ACE_Proper_Export_Flag
#    define MESSENGER_IOGR_IMR_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define MESSENGER_IOGR_IMR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* MESSENGER_IOGR_IMR_BUILD_DLL */
#    define Messenger_IOGR_Imr_Export ACE_Proper_Import_Flag
#    define MESSENGER_IOGR_IMR_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define MESSENGER_IOGR_IMR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* MESSENGER_IOGR_IMR_BUILD_DLL */
#else /* MESSENGER_IOGR_IMR_HAS_DLL == 1 */
#  define Messenger_IOGR_Imr_Export
#  define MESSENGER_IOGR_IMR_SINGLETON_DECLARATION(T)
#  define MESSENGER_IOGR_IMR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* MESSENGER_IOGR_IMR_HAS_DLL == 1 */

// Set MESSENGER_IOGR_IMR_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (MESSENGER_IOGR_IMR_NTRACE)
#  if (ACE_NTRACE == 1)
#    define MESSENGER_IOGR_IMR_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define MESSENGER_IOGR_IMR_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !MESSENGER_IOGR_IMR_NTRACE */

#if (MESSENGER_IOGR_IMR_NTRACE == 1)
#  define MESSENGER_IOGR_IMR_TRACE(X)
#else /* (MESSENGER_IOGR_IMR_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define MESSENGER_IOGR_IMR_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (MESSENGER_IOGR_IMR_NTRACE == 1) */

#endif /* MESSENGER_IOGR_IMR_EXPORT_H */

// End of auto generated file.
