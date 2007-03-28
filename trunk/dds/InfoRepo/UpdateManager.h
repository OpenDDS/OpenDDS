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

#include "Updater.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#include <set>

// forward declarations
class TAO_DDS_DCPSInfo_i;
class Updater;

class UpdateManager : public ACE_Service_Object
{
 public:
  typedef struct TopicStrt < ::DDS::TopicQos &, char*> UTopic;

  UpdateManager (void);

  virtual ~UpdateManager (void);

  // mechanism for InfoRepo object to be registered.
  void add (TAO_DDS_DCPSInfo_i* info);
  void add (Updater* updater);

  // Mechanism to register Updaters/InfoRepo
  void remove ();
  void remove (const Updater* updater);

  /// Force a clean shutdown.
  void shutdown (void);

  //
  // Methods inherited from the Updater
  //

  virtual void unregisterCallback (void);

  // Request an image refresh to be sent to
  //  the specified callback (asynchronously).
  virtual void requestImage (void);

  // Add entities to be persisted.
  virtual void add (const UTopic& topic);
  virtual void add(const ParticipantData& participant);
  virtual void add(const ActorData& actor);

  // Remove an entity (but not children) from persistence.
  virtual void remove(const ItemType& itemType, const IdType& id);

  // Persist updated Qos parameters for an entity.
  virtual void updateQos(const ItemType& itemType, const IdType& id
			 , const QosType& qos);

 private:
  typedef std::set <Updater*> Updaters;

  TAO_DDS_DCPSInfo_i* info_;
  Updaters updaters_;

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
