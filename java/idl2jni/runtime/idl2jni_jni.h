/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef idl2jni_jni_H
#define idl2jni_jni_H

// Sun's jni_md.h for linux doesn't take into account the fact that GCC 4 has
// hidden visibility as an option.  This header will redefine JNIEXPORT to
// whatever is appropriate for the platform, as configured by ACE and the user.
//
// Don't do this for Android as it doesn't have a jni_md.h file

#include <ace/config-all.h>

#ifndef ACE_ANDROID
#  include <jni_md.h>
#  undef JNIEXPORT
#  define JNIEXPORT ACE_Proper_Export_Flag
#endif

#include <jni.h>

#endif /* idl2jni_jni_H */
