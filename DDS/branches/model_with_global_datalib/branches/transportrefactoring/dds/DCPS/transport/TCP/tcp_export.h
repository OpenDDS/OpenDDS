
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Tcp
// ------------------------------
#ifndef TCP_EXPORT_H
#define TCP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TCP_HAS_DLL)
#  define TCP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TCP_HAS_DLL */

#if !defined (TCP_HAS_DLL)
#  define TCP_HAS_DLL 1
#endif /* ! TCP_HAS_DLL */

#if defined (TCP_HAS_DLL) && (TCP_HAS_DLL == 1)
#  if defined (TCP_BUILD_DLL)
#    define Tcp_Export ACE_Proper_Export_Flag
#    define TCP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TCP_BUILD_DLL */
#    define Tcp_Export ACE_Proper_Import_Flag
#    define TCP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TCP_BUILD_DLL */
#else /* TCP_HAS_DLL == 1 */
#  define Tcp_Export
#  define TCP_SINGLETON_DECLARATION(T)
#  define TCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TCP_HAS_DLL == 1 */

// Set TCP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TCP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TCP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TCP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TCP_NTRACE */

#if (TCP_NTRACE == 1)
#  define TCP_TRACE(X)
#else /* (TCP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TCP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TCP_NTRACE == 1) */

#endif /* TCP_EXPORT_H */

// End of auto generated file.
