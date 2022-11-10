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

#if defined (ACE_AS_STATIC_LIBS)
extern interface_instance_t static_be_plugin(const ACE_TString& key);
#endif

namespace {
  typedef std::vector<BE_Interface*> InterfaceList;
  InterfaceList interfaces;

  bool
  BE_register(const ACE_TString& dllname)
  {
    if (dllname == "") {
      interfaces.push_back(BE_BuiltinInterface::instance());
      return true;
    }
    else {
      ACE_DLL_Manager* manager = ACE_DLL_Manager::instance();
      if (manager != 0) {
        ACE_DLL_Handle* handle = manager->open_dll(dllname.c_str(), RTLD_NOW,
                                                   ACE_SHLIB_INVALID_HANDLE);
        interface_instance_t interface_instance = 0;
        if (handle != 0) {
          ACE_TString funcname(dllname);
          funcname += ACE_TEXT("_instance");
          interface_instance =
            reinterpret_cast<interface_instance_t>(
                                           handle->symbol(funcname.c_str()));
        }

#if defined (ACE_AS_STATIC_LIBS)
        if (interface_instance == 0) {
          interface_instance = static_be_plugin(dllname);
        }
#endif

        if (interface_instance == 0) {
          manager->close_dll(dllname.c_str());
        }
        else {
          BE_Interface* iface = interface_instance();
          if (iface != 0) {
            interfaces.push_back(iface);
            return true;
          }
        }
      }
    }
    return false;
  }

  int BE_pre_init(int& argc, ACE_TCHAR* argv[]) {
    int status = 0;
    bool plugin = false;
    for (int i = 1; i < argc; i++) {
      if (ACE_OS::strcmp(argv[i], ACE_TEXT("--plugin")) == 0) {
        if (++i < argc) {
          // Register the plugin
          const ACE_TString libname(argv[i]);
          if (BE_register(libname)) {
            plugin = true;
          }
          else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: Unable to load the plugin: %s\n"),
                       libname.c_str()));
            status++;
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
        else {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("ERROR: --plugin requires a library name parameter.\n")));
          status++;
        }
      }
    }

    // If we didn't load in a plugin, we will register the built-in backend.
    if (!plugin) {
      BE_register("");
    }

    // Allocate the fake be_global data which is expected to exist by tao_idl.
    // It doesn't actually do anything except call back into functions within
    // this file.
    if (be_global == 0) {
      ACE_NEW_RETURN(be_global, BE_GlobalData, 1);
    }

    return status;
  }
}

opendds_idl_plugin_Export void BE_version()
{
  InterfaceList::const_iterator end = interfaces.end();
  for (InterfaceList::const_iterator itr = interfaces.begin();
       itr != end; ++itr) {
    (*itr)->version();
  }
}

opendds_idl_plugin_Export int
BE_init(int& argc, ACE_TCHAR* argv[])
{
  int status = BE_pre_init(argc, argv);

  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    status |= (*itr)->init(argc, argv);
  }
  return status;
}

opendds_idl_plugin_Export void
BE_post_init(char* files[], long nfiles)
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->post_init(files, nfiles);
  }
}

opendds_idl_plugin_Export void BE_produce()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->produce();
  }
}

// Clean up before exit, whether successful or not.
// Need not be exported since it is called only from this file.
opendds_idl_plugin_Export void BE_cleanup()
{
  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->cleanup();
  }
}

// Abort this run of the BE.
opendds_idl_plugin_Export void BE_abort()
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
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" --plugin <libname>\tdynamically load a backend plugin\n")
    ));

  InterfaceList::iterator end = interfaces.end();
  for (InterfaceList::iterator itr = interfaces.begin(); itr != end; ++itr) {
    (*itr)->usage();
  }
}
