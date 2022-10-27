#ifndef OPENDDS_IDL_BE_BUILTIN_H
#define OPENDDS_IDL_BE_BUILTIN_H

#include "be_interface.h"
#include "be_builtin_global.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

extern BE_BuiltinGlobalData* be_builtin_global;

class BE_BuiltinInterface: public BE_Interface {
public:
  virtual ~BE_BuiltinInterface();

  // Extern BE_* functions

  virtual int init(int&, ACE_TCHAR*[]);
  virtual void post_init(char*[], long);
  virtual void version() const;
  virtual void produce();
  virtual void cleanup();

  // Externally called BE_GlobalData methods
  virtual void destroy();
  virtual void parse_args(long& i, char** av);
};

#endif /* OPENDDS_IDL_BE_BUILTIN_H */
