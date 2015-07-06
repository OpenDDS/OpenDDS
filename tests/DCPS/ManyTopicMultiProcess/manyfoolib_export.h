
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl ManyFooLib
// ------------------------------
#ifndef MANYFOOLIB_EXPORT_H
#define MANYFOOLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (MANYFOOLIB_HAS_DLL)
#  define MANYFOOLIB_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && MANYFOOLIB_HAS_DLL */

#if !defined (MANYFOOLIB_HAS_DLL)
#  define MANYFOOLIB_HAS_DLL 1
#endif /* ! MANYFOOLIB_HAS_DLL */

#if defined (MANYFOOLIB_HAS_DLL) && (MANYFOOLIB_HAS_DLL == 1)
#  if defined (MANYFOOLIB_BUILD_DLL)
#    define ManyFooLib_Export ACE_Proper_Export_Flag
#    define MANYFOOLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define MANYFOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* MANYFOOLIB_BUILD_DLL */
#    define ManyFooLib_Export ACE_Proper_Import_Flag
#    define MANYFOOLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define MANYFOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* MANYFOOLIB_BUILD_DLL */
#else /* MANYFOOLIB_HAS_DLL == 1 */
#  define ManyFooLib_Export
#  define MANYFOOLIB_SINGLETON_DECLARATION(T)
#  define MANYFOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* MANYFOOLIB_HAS_DLL == 1 */

// Set MANYFOOLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (MANYFOOLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define MANYFOOLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define MANYFOOLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !MANYFOOLIB_NTRACE */

#if (MANYFOOLIB_NTRACE == 1)
#  define MANYFOOLIB_TRACE(X)
#else /* (MANYFOOLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define MANYFOOLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (MANYFOOLIB_NTRACE == 1) */

#endif /* MANYFOOLIB_EXPORT_H */

// End of auto generated file.
