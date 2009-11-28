
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s DummyTcp
// ------------------------------
#ifndef DUMMYTCP_EXPORT_H
#define DUMMYTCP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DUMMYTCP_HAS_DLL)
#  define DUMMYTCP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DUMMYTCP_HAS_DLL */

#if !defined (DUMMYTCP_HAS_DLL)
#  define DUMMYTCP_HAS_DLL 1
#endif /* ! DUMMYTCP_HAS_DLL */

#if defined (DUMMYTCP_HAS_DLL) && (DUMMYTCP_HAS_DLL == 1)
#  if defined (DUMMYTCP_BUILD_DLL)
#    define DummyTcp_Export ACE_Proper_Export_Flag
#    define DUMMYTCP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DUMMYTCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DUMMYTCP_BUILD_DLL */
#    define DummyTcp_Export ACE_Proper_Import_Flag
#    define DUMMYTCP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DUMMYTCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DUMMYTCP_BUILD_DLL */
#else /* DUMMYTCP_HAS_DLL == 1 */
#  define DummyTcp_Export
#  define DUMMYTCP_SINGLETON_DECLARATION(T)
#  define DUMMYTCP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DUMMYTCP_HAS_DLL == 1 */

// Set DUMMYTCP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DUMMYTCP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DUMMYTCP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DUMMYTCP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DUMMYTCP_NTRACE */

#if (DUMMYTCP_NTRACE == 1)
#  define DUMMYTCP_TRACE(X)
#else /* (DUMMYTCP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DUMMYTCP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DUMMYTCP_NTRACE == 1) */

#endif /* DUMMYTCP_EXPORT_H */

// End of auto generated file.
