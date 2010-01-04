/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef IDL2JNI_RT_15A_COMPAT_H
#define IDL2JNI_RT_15A_COMPAT_H

#include "tao/Version.h"

#if (TAO_MAJOR_VERSION == 1 && \
 TAO_MINOR_VERSION == 5 && TAO_BETA_VERSION <= 6)
#  define IDL2JNI_NO_INC_SCSET_H
#  define IDL2JNI_CONST_SEQ_ELEM charstr_sequence_element
#else
#  define IDL2JNI_CONST_SEQ_ELEM string_const_sequence_element<char_string_traits>
#endif //TAO version

#endif //header guard
