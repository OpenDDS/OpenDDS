/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_extern.h"
#include "be_builtin.h"

namespace {
  typedef std::vector<BE_Interface*> InterfaceList;
  InterfaceList interfaces;

  void register_builtin() {
    static bool registered = false;
    if (!registered) {
      // Register the builtin backend
      BE_Interface* builtin = 0;
      ACE_NEW(builtin, BE_BuiltinInterface);
      if (builtin != 0) {
        BE_register(builtin);
        registered = true;
      }
    }

    // Allocate the fake be_global data which is expected to exist by tao_idl.
    // It doesn't actually do anything except call back into functions within this file.
    if (be_global == 0) {
      ACE_NEW(be_global, BE_GlobalData);
    }
  }
}

void
BE_register(BE_Interface* interface)
{
  interfaces.push_back(interface);
}

void
BE_version()
{
  InterfaceList::const_iterator end = interfaces.end();
  for (InterfaceList::const_iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->version();
  }
}

int
BE_init(int& argc, ACE_TCHAR* argv[])
{
  register_builtin();

  int status = 0;
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    status |= (*itr)->init(argc, argv);
  }
  return status;
}

void
BE_post_init(char* files[], long nfiles)
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->post_init(files, nfiles);
  }
}

void
BE_produce()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->produce();
  }
}

// Clean up before exit, whether successful or not.
// Need not be exported since it is called only from this file.
void
BE_cleanup()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->cleanup();
  }
}

// Abort this run of the BE.
void
BE_abort()
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("Fatal Error - Aborting\n")));
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->cleanup();
  }
  ACE_OS::exit(1);
}

void BE_destroy()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->destroy();
  }
}

void BE_parse_args(long& i, char** av)
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->parse_args(i, av);
  }
}
