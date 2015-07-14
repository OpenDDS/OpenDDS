
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl DDSWrapper
// ------------------------------
#ifndef DDSWRAPPER_EXPORT_H
#define DDSWRAPPER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DDSWRAPPER_HAS_DLL)
#  define DDSWRAPPER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DDSWRAPPER_HAS_DLL */

#if !defined (DDSWRAPPER_HAS_DLL)
#  define DDSWRAPPER_HAS_DLL 1
#endif /* ! DDSWRAPPER_HAS_DLL */

#if defined (DDSWRAPPER_HAS_DLL) && (DDSWRAPPER_HAS_DLL == 1)
#  if defined (DDSWRAPPER_BUILD_DLL)
#    define DDSWrapper_Export ACE_Proper_Export_Flag
#    define DDSWRAPPER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DDSWRAPPER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DDSWRAPPER_BUILD_DLL */
#    define DDSWrapper_Export ACE_Proper_Import_Flag
#    define DDSWRAPPER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DDSWRAPPER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DDSWRAPPER_BUILD_DLL */
#else /* DDSWRAPPER_HAS_DLL == 1 */
#  define DDSWrapper_Export
#  define DDSWRAPPER_SINGLETON_DECLARATION(T)
#  define DDSWRAPPER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DDSWRAPPER_HAS_DLL == 1 */

// Set DDSWRAPPER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DDSWRAPPER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DDSWRAPPER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DDSWRAPPER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DDSWRAPPER_NTRACE */

#if (DDSWRAPPER_NTRACE == 1)
#  define DDSWRAPPER_TRACE(X)
#else /* (DDSWRAPPER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DDSWRAPPER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DDSWRAPPER_NTRACE == 1) */

#endif /* DDSWRAPPER_EXPORT_H */

// End of auto generated file.
