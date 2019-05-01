
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Stool
// ------------------------------
#ifndef STOOL_EXPORT_H
#define STOOL_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (STOOL_HAS_DLL)
#  define STOOL_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && STOOL_HAS_DLL */

#if !defined (STOOL_HAS_DLL)
#  define STOOL_HAS_DLL 1
#endif /* ! STOOL_HAS_DLL */

#if defined (STOOL_HAS_DLL) && (STOOL_HAS_DLL == 1)
#  if defined (STOOL_BUILD_DLL)
#    define Stool_Export ACE_Proper_Export_Flag
#    define STOOL_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define STOOL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* STOOL_BUILD_DLL */
#    define Stool_Export ACE_Proper_Import_Flag
#    define STOOL_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define STOOL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* STOOL_BUILD_DLL */
#else /* STOOL_HAS_DLL == 1 */
#  define Stool_Export
#  define STOOL_SINGLETON_DECLARATION(T)
#  define STOOL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* STOOL_HAS_DLL == 1 */

// Set STOOL_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (STOOL_NTRACE)
#  if (ACE_NTRACE == 1)
#    define STOOL_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define STOOL_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !STOOL_NTRACE */

#if (STOOL_NTRACE == 1)
#  define STOOL_TRACE(X)
#else /* (STOOL_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define STOOL_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (STOOL_NTRACE == 1) */

#endif /* STOOL_EXPORT_H */

// End of auto generated file.
