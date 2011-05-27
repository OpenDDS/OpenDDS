
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl MCast
// ------------------------------
#ifndef MCAST_EXPORT_H
#define MCAST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (MCAST_HAS_DLL)
#  define MCAST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && MCAST_HAS_DLL */

#if !defined (MCAST_HAS_DLL)
#  define MCAST_HAS_DLL 1
#endif /* ! MCAST_HAS_DLL */

#if defined (MCAST_HAS_DLL) && (MCAST_HAS_DLL == 1)
#  if defined (MCAST_BUILD_DLL)
#    define MCast_Export ACE_Proper_Export_Flag
#    define MCAST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define MCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* MCAST_BUILD_DLL */
#    define MCast_Export ACE_Proper_Import_Flag
#    define MCAST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define MCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* MCAST_BUILD_DLL */
#else /* MCAST_HAS_DLL == 1 */
#  define MCast_Export
#  define MCAST_SINGLETON_DECLARATION(T)
#  define MCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* MCAST_HAS_DLL == 1 */

// Set MCAST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (MCAST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define MCAST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define MCAST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !MCAST_NTRACE */

#if (MCAST_NTRACE == 1)
#  define MCAST_TRACE(X)
#else /* (MCAST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define MCAST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (MCAST_NTRACE == 1) */

#endif /* MCAST_EXPORT_H */

// End of auto generated file.
