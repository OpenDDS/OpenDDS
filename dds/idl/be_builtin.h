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

  // Provide a language mapping
  LanguageMapping* language_mapping();

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

protected:
  static void rm_arg(int& i, int& argc, ACE_TCHAR* argv[]);
  void allocate_language_mapping(int&, ACE_TCHAR* []);
};

#endif /* OPENDDS_IDL_BE_BUILTIN_H */
