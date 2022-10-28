#ifndef OPENDDS_IDL_BE_INTERFACE_H
#define OPENDDS_IDL_BE_INTERFACE_H

#include <ace/SString.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class BE_Interface {
public:
  BE_Interface()
    : be_abort(0),
      drv_cpp_putarg(0) {
  }
  virtual ~BE_Interface() {}

  // Extern BE_* functions

  virtual int init(int&, ACE_TCHAR*[]) = 0;
  virtual void post_init(char*[], long) = 0;
  virtual void version() const = 0;
  virtual void produce() = 0;
  virtual void cleanup() = 0;

  // Externally called BE_GlobalData methods
  virtual void destroy() = 0;
  virtual void parse_args(long& i, char** av) = 0;

  // Called by be_util
  virtual void prep_be_arg(char* arg) = 0;
  virtual void arg_post_proc() = 0;
  virtual void usage() = 0;

  // To be used by plugins
  typedef void (*abort_ptr)();
  typedef void (*drv_putarg_ptr)(const char*);
  abort_ptr be_abort;
  drv_putarg_ptr drv_cpp_putarg;

  // Called during registration to set function pointers in the loaded plugin.
  void post_registration(abort_ptr be_abort_func,
                         drv_putarg_ptr drv_cpp_putarg_func) {
    be_abort = be_abort_func;
    drv_cpp_putarg = drv_cpp_putarg_func;
  }
};

#endif /* OPENDDS_IDL_BE_INTERFACE_H */
