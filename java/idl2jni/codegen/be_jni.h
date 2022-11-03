/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_JNI_H
#define OPENDDS_IDL_BE_JNI_H

#include "be_jni_global.h"
#include <be_interface.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

extern BE_JNIGlobalData* be_jni_global;

class BE_JNIInterface: public BE_Interface {
public:
  virtual ~BE_JNIInterface();

  // Extern BE_* functions

  virtual int init(int&, ACE_TCHAR*[]);
  virtual void post_init(char*[], long);
  virtual void version() const;
  virtual void produce();
  virtual void cleanup();

  // Externally called BE_GlobalData methods
  virtual void destroy();
  virtual void parse_args(long& i, char** av);

  // Called by be_util
  virtual void prep_be_arg(char* arg);
  virtual void arg_post_proc();
  virtual void usage();
};

#endif /* OPENDDS_IDL_BE_JNI_H */
