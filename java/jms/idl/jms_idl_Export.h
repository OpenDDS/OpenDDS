// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl jms_idl
// ------------------------------
#ifndef JMS_IDL_EXPORT_H
#define JMS_IDL_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (JMS_IDL_HAS_DLL)
#  define JMS_IDL_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && JMS_IDL_HAS_DLL */

#if !defined (JMS_IDL_HAS_DLL)
#  define JMS_IDL_HAS_DLL 1
#endif /* ! JMS_IDL_HAS_DLL */

#if defined (JMS_IDL_HAS_DLL) && (JMS_IDL_HAS_DLL == 1)
#  if defined (JMS_IDL_BUILD_DLL)
#    define jms_idl_Export ACE_Proper_Export_Flag
#    define JMS_IDL_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define JMS_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* JMS_IDL_BUILD_DLL */
#    define jms_idl_Export ACE_Proper_Import_Flag
#    define JMS_IDL_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define JMS_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* JMS_IDL_BUILD_DLL */
#else /* JMS_IDL_HAS_DLL == 1 */
#  define jms_idl_Export
#  define JMS_IDL_SINGLETON_DECLARATION(T)
#  define JMS_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* JMS_IDL_HAS_DLL == 1 */

// Set JMS_IDL_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (JMS_IDL_NTRACE)
#  if (ACE_NTRACE == 1)
#    define JMS_IDL_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define JMS_IDL_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !JMS_IDL_NTRACE */

#if (JMS_IDL_NTRACE == 1)
#  define JMS_IDL_TRACE(X)
#else /* (JMS_IDL_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define JMS_IDL_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (JMS_IDL_NTRACE == 1) */

#endif /* JMS_IDL_EXPORT_H */

// End of auto generated file.
