
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Udp
// ------------------------------
#ifndef UDP_EXPORT_H
#define UDP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (UDP_HAS_DLL)
#  define UDP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && UDP_HAS_DLL */

#if !defined (UDP_HAS_DLL)
#  define UDP_HAS_DLL 1
#endif /* ! UDP_HAS_DLL */

#if defined (UDP_HAS_DLL) && (UDP_HAS_DLL == 1)
#  if defined (UDP_BUILD_DLL)
#    define Udp_Export ACE_Proper_Export_Flag
#    define UDP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* UDP_BUILD_DLL */
#    define Udp_Export ACE_Proper_Import_Flag
#    define UDP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* UDP_BUILD_DLL */
#else /* UDP_HAS_DLL == 1 */
#  define Udp_Export
#  define UDP_SINGLETON_DECLARATION(T)
#  define UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* UDP_HAS_DLL == 1 */

// Set UDP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (UDP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define UDP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define UDP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !UDP_NTRACE */

#if (UDP_NTRACE == 1)
#  define UDP_TRACE(X)
#else /* (UDP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define UDP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (UDP_NTRACE == 1) */

#endif /* UDP_EXPORT_H */

// End of auto generated file.
