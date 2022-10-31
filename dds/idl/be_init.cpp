/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_init.h"
#include "be_extern.h"
#include "be_builtin.h"

#include <ace/DLL_Manager.h>
#include <drv_extern.h>

namespace {
  typedef std::vector<BE_Interface*> InterfaceList;
  InterfaceList interfaces;

  void BE_pre_init(int& argc, ACE_TCHAR* argv[]) {
    bool plugin = false;
    for (int i = 1; i < argc; i++) {
      if (ACE_OS::strcmp(argv[i], ACE_TEXT("--plugin")) == 0) {
        if (++i < argc) {
          // Register the plugin
          const ACE_TString libname(argv[i]);
          ACE_TString allocator(libname);
          allocator += ACE_TEXT("_allocator");
          if (BE_register(libname.c_str(), allocator.c_str())) {
            plugin = true;
          }
          else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: Unable to load the plugin: %s\n"),
                       libname.c_str()));
          }

          // Shift everything down two (including the NULL)
          const int down = 2;
          for (int j = i + 1; j <= argc; j++) {
            argv[j - down] = argv[j];
          }
          argc -= down;

          // Set i back to where it was when it was on the current option
          // after the next increment in the for loop.
          i -= down;
        }
      }
    }

    // If we didn't load in a plugin, we will register the built-in backend.
    if (!plugin) {
      BE_register(0, 0);
    }

    // Allocate the fake be_global data which is expected to exist by tao_idl.
    // It doesn't actually do anything except call back into functions within
    // this file.
    if (be_global == 0) {
      ACE_NEW(be_global, BE_GlobalData);
    }
  }
}

bool
BE_register(const ACE_TCHAR* dllname, const ACE_TCHAR* allocator)
{
  if (dllname == 0 || ACE_OS::strcmp(dllname, "") == 0) {
    BE_BuiltinInterface* builtin = 0;
    ACE_NEW_RETURN(builtin, BE_BuiltinInterface, false);
    if (builtin != 0) {
      builtin->post_registration(BE_abort, DRV_cpp_putarg);
      interfaces.push_back(builtin);
      return true;
    }
  }
  else {
    ACE_DLL_Manager* manager = ACE_DLL_Manager::instance();
    if (manager != 0) {
      ACE_DLL_Handle* handle = manager->open_dll(dllname, RTLD_NOW,
                                                 ACE_SHLIB_INVALID_HANDLE);
      if (handle != 0) {
        typedef BE_Interface* (*interface_alloc_t)();
        const interface_alloc_t interface_alloc =
          reinterpret_cast<interface_alloc_t>(allocator == 0 ?
                                              0 : handle->symbol(allocator));
        if (interface_alloc == 0) {
          manager->close_dll(dllname);
        }
        else {
          BE_Interface* iface = interface_alloc();
          if (iface != 0) {
            iface->post_registration(BE_abort, DRV_cpp_putarg);
            interfaces.push_back(iface);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void
BE_version()
{
  InterfaceList::const_iterator end = interfaces.end();
  for (InterfaceList::const_iterator itr = interfaces.begin();
       itr != end; ++itr) {
    (*itr)->version();
  }
}

int
BE_init(int& argc, ACE_TCHAR* argv[])
{
  BE_pre_init(argc, argv);

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

void BE_prep_be_arg(char* arg)
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->prep_be_arg(arg);
  }
}

void BE_arg_post_proc()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->arg_post_proc();
  }
}

void BE_usage()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->usage();
  }
}
