
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl DdsSecurity
// ------------------------------
#ifndef DDSSECURITY_EXPORT_H
#define DDSSECURITY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DDSSECURITY_HAS_DLL)
#  define DDSSECURITY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DDSSECURITY_HAS_DLL */

#if !defined (DDSSECURITY_HAS_DLL)
#  define DDSSECURITY_HAS_DLL 1
#endif /* ! DDSSECURITY_HAS_DLL */

#if defined (DDSSECURITY_HAS_DLL) && (DDSSECURITY_HAS_DLL == 1)
#  if defined (DDSSECURITY_BUILD_DLL)
#    define DdsSecurity_Export ACE_Proper_Export_Flag
#    define DDSSECURITY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DDSSECURITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DDSSECURITY_BUILD_DLL */
#    define DdsSecurity_Export ACE_Proper_Import_Flag
#    define DDSSECURITY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DDSSECURITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DDSSECURITY_BUILD_DLL */
#else /* DDSSECURITY_HAS_DLL == 1 */
#  define DdsSecurity_Export
#  define DDSSECURITY_SINGLETON_DECLARATION(T)
#  define DDSSECURITY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DDSSECURITY_HAS_DLL == 1 */

// Set DDSSECURITY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DDSSECURITY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DDSSECURITY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DDSSECURITY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DDSSECURITY_NTRACE */

#if (DDSSECURITY_NTRACE == 1)
#  define DDSSECURITY_TRACE(X)
#else /* (DDSSECURITY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DDSSECURITY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DDSSECURITY_NTRACE == 1) */

#endif /* DDSSECURITY_EXPORT_H */

// End of auto generated file.
