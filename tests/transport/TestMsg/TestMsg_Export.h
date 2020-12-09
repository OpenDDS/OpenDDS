
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl TestMsg
// ------------------------------
#ifndef TESTMSG_EXPORT_H
#define TESTMSG_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TESTMSG_HAS_DLL)
#  define TESTMSG_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TESTMSG_HAS_DLL */

#if !defined (TESTMSG_HAS_DLL)
#  define TESTMSG_HAS_DLL 1
#endif /* ! TESTMSG_HAS_DLL */

#if defined (TESTMSG_HAS_DLL) && (TESTMSG_HAS_DLL == 1)
#  if defined (TESTMSG_BUILD_DLL)
#    define TestMsg_Export ACE_Proper_Export_Flag
#    define TESTMSG_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TESTMSG_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TESTMSG_BUILD_DLL */
#    define TestMsg_Export ACE_Proper_Import_Flag
#    define TESTMSG_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TESTMSG_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TESTMSG_BUILD_DLL */
#else /* TESTMSG_HAS_DLL == 1 */
#  define TestMsg_Export
#  define TESTMSG_SINGLETON_DECLARATION(T)
#  define TESTMSG_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TESTMSG_HAS_DLL == 1 */

// Set TESTMSG_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TESTMSG_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TESTMSG_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TESTMSG_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TESTMSG_NTRACE */

#if (TESTMSG_NTRACE == 1)
#  define TESTMSG_TRACE(X)
#else /* (TESTMSG_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TESTMSG_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TESTMSG_NTRACE == 1) */

#endif /* TESTMSG_EXPORT_H */

// End of auto generated file.
