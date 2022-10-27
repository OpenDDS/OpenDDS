#ifndef OPENDDS_IDL_BE_INTERFACE_H
#define OPENDDS_IDL_BE_INTERFACE_H

#include <ace/SString.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class BE_Interface {
public:
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
};

#endif /* OPENDDS_IDL_BE_INTERFACE_H */
