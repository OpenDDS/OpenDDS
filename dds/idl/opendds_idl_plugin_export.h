
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl opendds_idl_plugin
// ------------------------------
#ifndef OPENDDS_IDL_PLUGIN_EXPORT_H
#define OPENDDS_IDL_PLUGIN_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OPENDDS_IDL_PLUGIN_HAS_DLL)
#  define OPENDDS_IDL_PLUGIN_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OPENDDS_IDL_PLUGIN_HAS_DLL */

#if !defined (OPENDDS_IDL_PLUGIN_HAS_DLL)
#  define OPENDDS_IDL_PLUGIN_HAS_DLL 1
#endif /* ! OPENDDS_IDL_PLUGIN_HAS_DLL */

#if defined (OPENDDS_IDL_PLUGIN_HAS_DLL) && (OPENDDS_IDL_PLUGIN_HAS_DLL == 1)
#  if defined (OPENDDS_IDL_PLUGIN_BUILD_DLL)
#    define opendds_idl_plugin_Export ACE_Proper_Export_Flag
#    define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OPENDDS_IDL_PLUGIN_BUILD_DLL */
#    define opendds_idl_plugin_Export ACE_Proper_Import_Flag
#    define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OPENDDS_IDL_PLUGIN_BUILD_DLL */
#else /* OPENDDS_IDL_PLUGIN_HAS_DLL == 1 */
#  define opendds_idl_plugin_Export
#  define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARATION(T)
#  define OPENDDS_IDL_PLUGIN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OPENDDS_IDL_PLUGIN_HAS_DLL == 1 */

// Set OPENDDS_IDL_PLUGIN_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OPENDDS_IDL_PLUGIN_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OPENDDS_IDL_PLUGIN_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OPENDDS_IDL_PLUGIN_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OPENDDS_IDL_PLUGIN_NTRACE */

#if (OPENDDS_IDL_PLUGIN_NTRACE == 1)
#  define OPENDDS_IDL_PLUGIN_TRACE(X)
#else /* (OPENDDS_IDL_PLUGIN_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OPENDDS_IDL_PLUGIN_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OPENDDS_IDL_PLUGIN_NTRACE == 1) */

#endif /* OPENDDS_IDL_PLUGIN_EXPORT_H */

// End of auto generated file.
