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

class Updater
{
 public:

  virtual ~Updater (void);

  virtual void unregisterCallback (void) = 0;

  // Request an image refresh to be sent to
  //  the specified callback (asynchronously).
  virtual void requestImage (void) = 0;

  // Add entities to be persisted.
  virtual void add(const TopicData& topic) = 0;
  virtual void add(const ParticipantData& participant) = 0;
  virtual void add(const ActorData& actor) = 0;

  // Remove an entity (but not children) from persistence.
  virtual void remove(const ItemType& itemType, const IdType& id) = 0;

  // Persist updated Qos parameters for an entity.
  virtual void updateQos(const ItemType& itemType, const IdType& id
			 , const QosType& qos) = 0;
};

// forward declare
class UpdateManager;

class UpdaterBase : public Updater
{
 public:
  UpdaterBase (void);
  virtual ~UpdaterBase (void);

  void unregisterCallback (void);

 protected:
  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  UpdateManager* um_;
  LockType um_lock_;
};

#endif // _UPDATER_
