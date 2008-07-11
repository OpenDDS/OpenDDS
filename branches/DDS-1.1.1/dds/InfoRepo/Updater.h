// -*- C++ -*-

/**
 * @file      Updater.h
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */

#ifndef _UPDATER_
#define _UPDATER_

#include "UpdateDataTypes.h"
#include "UpdateManager.h"

#include "ace/Synch.h"


class Updater
{
 public:

  virtual ~Updater (void);

  virtual void unregisterCallback (void) = 0;

  // Request an image refresh to be sent to
  //  the specified callback (asynchronously).
  virtual void requestImage (void) = 0;

  // Add entities to be persisted.
  virtual void add(const UpdateManager::DTopic& topic) = 0;
  virtual void add(const UpdateManager::DParticipant& participant) = 0;
  virtual void add(const UpdateManager::DActor& actor) = 0;

  // Remove an entity (but not children) from persistence.
  virtual void remove (ItemType type, const IdType& id) = 0;

  // Persist updated Qos parameters for an entity.
  virtual void updateQos(const ItemType& itemType, const IdType& id
			 , const QosSeq& qos) = 0;
};

// forward declare
class UpdateManager;

class UpdaterBase : public Updater
{
 public:
  virtual ~UpdaterBase (void);

  void unregisterCallback (void);

 protected:
  UpdateManager* um_;
};

#endif // _UPDATER_
