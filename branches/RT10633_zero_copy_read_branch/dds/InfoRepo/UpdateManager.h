// -*- C++ -*-

/**
 * @file      UpdateManager.h
 *
 * @library   UpdateManager
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */


#ifndef _UPDATE_MANAGER_
#define _UPDATE_MANAGER_

#include "ace/Task.h"
#include "ace/Service_Config.h"

// forward declarations
class TAO_DDS_DCPSInfo_i;

class UpdateManager : public ACE_Task_Base
{
 public:
  UpdateManager (void);

  virtual ~UpdateManager (void);

  // mechanism for InfoRepo object to be registered.
  void add (TAO_DDS_DCPSInfo_i* info);

  virtual int svc (void);

  /// Force a clean shutdown.
  void shutdown (void);

  // Mechanism to register Updaters.

 private:
  TAO_DDS_DCPSInfo_i* info_;

  // This isn't intended to be a shared library.
  //  Hiding these interfaces.
  /// Shared object initializer
  int init (int argc, ACE_TCHAR *argv[]);

  /// Shared object finalizer
  int fini (void);
};

ACE_STATIC_SVC_DECLARE (UpdateManager)
ACE_FACTORY_DECLARE (ACE_Local_Service, UpdateManager)

class UpdateManager_Loader
{
public:
  static int init (void);
};

#if defined(ACE_HAS_BROKEN_STATIC_CONSTRUCTORS)

typedef int (*UpdateManager_Loader) (void);

static UpdateManager_Loader ldr =
&UpdateManager_Loader::init;

#else

static int ldr =
UpdateManager_Loader::init ();

#endif /* ACE_HAS_BROKEN_STATIC_CONSTRUCTORS */

#endif // _UPDATE_MANAGER_
