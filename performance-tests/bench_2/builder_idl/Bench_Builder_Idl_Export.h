
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Bench_Builder_Idl
// ------------------------------
#ifndef BENCH_BUILDER_IDL_EXPORT_H
#define BENCH_BUILDER_IDL_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (BENCH_BUILDER_IDL_HAS_DLL)
#  define BENCH_BUILDER_IDL_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && BENCH_BUILDER_IDL_HAS_DLL */

#if !defined (BENCH_BUILDER_IDL_HAS_DLL)
#  define BENCH_BUILDER_IDL_HAS_DLL 1
#endif /* ! BENCH_BUILDER_IDL_HAS_DLL */

#if defined (BENCH_BUILDER_IDL_HAS_DLL) && (BENCH_BUILDER_IDL_HAS_DLL == 1)
#  if defined (BENCH_BUILDER_IDL_BUILD_DLL)
#    define Bench_Builder_Idl_Export ACE_Proper_Export_Flag
#    define BENCH_BUILDER_IDL_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define BENCH_BUILDER_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* BENCH_BUILDER_IDL_BUILD_DLL */
#    define Bench_Builder_Idl_Export ACE_Proper_Import_Flag
#    define BENCH_BUILDER_IDL_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define BENCH_BUILDER_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* BENCH_BUILDER_IDL_BUILD_DLL */
#else /* BENCH_BUILDER_IDL_HAS_DLL == 1 */
#  define Bench_Builder_Idl_Export
#  define BENCH_BUILDER_IDL_SINGLETON_DECLARATION(T)
#  define BENCH_BUILDER_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* BENCH_BUILDER_IDL_HAS_DLL == 1 */

// Set BENCH_BUILDER_IDL_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (BENCH_BUILDER_IDL_NTRACE)
#  if (ACE_NTRACE == 1)
#    define BENCH_BUILDER_IDL_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define BENCH_BUILDER_IDL_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !BENCH_BUILDER_IDL_NTRACE */

#if (BENCH_BUILDER_IDL_NTRACE == 1)
#  define BENCH_BUILDER_IDL_TRACE(X)
#else /* (BENCH_BUILDER_IDL_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define BENCH_BUILDER_IDL_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (BENCH_BUILDER_IDL_NTRACE == 1) */

#endif /* BENCH_BUILDER_IDL_EXPORT_H */

// End of auto generated file.
