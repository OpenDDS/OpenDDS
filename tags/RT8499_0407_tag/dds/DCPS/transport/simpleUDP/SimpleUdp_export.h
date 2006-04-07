
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s SimpleUdp
// ------------------------------
#ifndef SIMPLEUDP_EXPORT_H
#define SIMPLEUDP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SIMPLEUDP_HAS_DLL)
#  define SIMPLEUDP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SIMPLEUDP_HAS_DLL */

#if !defined (SIMPLEUDP_HAS_DLL)
#  define SIMPLEUDP_HAS_DLL 1
#endif /* ! SIMPLEUDP_HAS_DLL */

#if defined (SIMPLEUDP_HAS_DLL) && (SIMPLEUDP_HAS_DLL == 1)
#  if defined (SIMPLEUDP_BUILD_DLL)
#    define SimpleUdp_Export ACE_Proper_Export_Flag
#    define SIMPLEUDP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SIMPLEUDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SIMPLEUDP_BUILD_DLL */
#    define SimpleUdp_Export ACE_Proper_Import_Flag
#    define SIMPLEUDP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SIMPLEUDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SIMPLEUDP_BUILD_DLL */
#else /* SIMPLEUDP_HAS_DLL == 1 */
#  define SimpleUdp_Export
#  define SIMPLEUDP_SINGLETON_DECLARATION(T)
#  define SIMPLEUDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SIMPLEUDP_HAS_DLL == 1 */

// Set SIMPLEUDP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SIMPLEUDP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SIMPLEUDP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SIMPLEUDP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SIMPLEUDP_NTRACE */

#if (SIMPLEUDP_NTRACE == 1)
#  define SIMPLEUDP_TRACE(X)
#else /* (SIMPLEUDP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SIMPLEUDP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SIMPLEUDP_NTRACE == 1) */

#endif /* SIMPLEUDP_EXPORT_H */

// End of auto generated file.
