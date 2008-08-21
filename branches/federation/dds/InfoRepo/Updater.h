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

#include "dds/DCPS/GuidUtils.h"
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
  virtual void add(const UpdateManager::UTopic& topic) = 0;
  virtual void add(const UpdateManager::UParticipant& participant) = 0;
  virtual void add(const UpdateManager::URActor& actor) = 0;
  virtual void add(const UpdateManager::UWActor& actor) = 0;
  virtual void add( const long domain, const OpenDDS::DCPS::GUID_t participant, const long owner) = 0;

  // Remove an entity (but not children) from persistence.
  virtual void remove(
                 ItemType      type,
                 const IdType& id,
                 ActorType     actor = DataWriter,
                 long          domain = 0,
                 const IdType& participant = ::OpenDDS::DCPS::GUID_UNKNOWN
               ) = 0;

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
  void add( const long domain, const OpenDDS::DCPS::GUID_t participant, const long owner);

 protected:
  UpdateManager* um_;
};

#endif // _UPDATER_
