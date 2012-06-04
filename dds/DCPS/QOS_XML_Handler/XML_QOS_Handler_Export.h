
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl XML_QOS_Handler
// ------------------------------
#ifndef XML_QOS_HANDLER_EXPORT_H
#define XML_QOS_HANDLER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (XML_QOS_HANDLER_HAS_DLL)
#  define XML_QOS_HANDLER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && XML_QOS_HANDLER_HAS_DLL */

#if !defined (XML_QOS_HANDLER_HAS_DLL)
#  define XML_QOS_HANDLER_HAS_DLL 1
#endif /* ! XML_QOS_HANDLER_HAS_DLL */

#if defined (XML_QOS_HANDLER_HAS_DLL) && (XML_QOS_HANDLER_HAS_DLL == 1)
#  if defined (XML_QOS_HANDLER_BUILD_DLL)
#    define XML_QOS_Handler_Export ACE_Proper_Export_Flag
#    define XML_QOS_HANDLER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define XML_QOS_HANDLER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* XML_QOS_HANDLER_BUILD_DLL */
#    define XML_QOS_Handler_Export ACE_Proper_Import_Flag
#    define XML_QOS_HANDLER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define XML_QOS_HANDLER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* XML_QOS_HANDLER_BUILD_DLL */
#else /* XML_QOS_HANDLER_HAS_DLL == 1 */
#  define XML_QOS_Handler_Export
#  define XML_QOS_HANDLER_SINGLETON_DECLARATION(T)
#  define XML_QOS_HANDLER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* XML_QOS_HANDLER_HAS_DLL == 1 */

// Set XML_QOS_HANDLER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (XML_QOS_HANDLER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define XML_QOS_HANDLER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define XML_QOS_HANDLER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !XML_QOS_HANDLER_NTRACE */

#if (XML_QOS_HANDLER_NTRACE == 1)
#  define XML_QOS_HANDLER_TRACE(X)
#else /* (XML_QOS_HANDLER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define XML_QOS_HANDLER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (XML_QOS_HANDLER_NTRACE == 1) */

#endif /* XML_QOS_HANDLER_EXPORT_H */

// End of auto generated file.
