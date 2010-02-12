
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl odds
// ------------------------------
#ifndef ODDS_EXPORT_H
#define ODDS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (ODDS_HAS_DLL)
#  define ODDS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && ODDS_HAS_DLL */

#if !defined (ODDS_HAS_DLL)
#  define ODDS_HAS_DLL 1
#endif /* ! ODDS_HAS_DLL */

#if defined (ODDS_HAS_DLL) && (ODDS_HAS_DLL == 1)
#  if defined (ODDS_BUILD_DLL)
#    define odds_Export ACE_Proper_Export_Flag
#    define ODDS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define ODDS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* ODDS_BUILD_DLL */
#    define odds_Export ACE_Proper_Import_Flag
#    define ODDS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define ODDS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* ODDS_BUILD_DLL */
#else /* ODDS_HAS_DLL == 1 */
#  define odds_Export
#  define ODDS_SINGLETON_DECLARATION(T)
#  define ODDS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* ODDS_HAS_DLL == 1 */

// Set ODDS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (ODDS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define ODDS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define ODDS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !ODDS_NTRACE */

#if (ODDS_NTRACE == 1)
#  define ODDS_TRACE(X)
#else /* (ODDS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define ODDS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (ODDS_NTRACE == 1) */

#endif /* ODDS_EXPORT_H */

// End of auto generated file.
