// -*- C++ -*-

/**
 * @file      PersistenceUpdate.h
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */

#ifndef _PERSISTENCE_UPDATER_
#define _PERSISTENCE_UPDATER_

#include "Updater.h"

#include "ace/Task.h"
#include "ace/Hash_Map_With_Allocator_T.h"
#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"

#include <string>

class UpdateManager;

class PersistenceUpdater : public UpdaterBase, public ACE_Task_Base
{
 public:
  class IdType_ExtId
  {
  public:
    IdType_ExtId ();

    IdType_ExtId (IdType id);

    IdType_ExtId (const IdType_ExtId& ext);

    void operator= (const IdType_ExtId& ext);

    bool operator== (const IdType_ExtId& ext) const;

    unsigned long hash (void) const;

  private:
    IdType id_;
  };

  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, TopicData*> TopicIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, ParticipantData*> ParticipantIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, ActorData*> ActorIndex;

  typedef ACE_Allocator_Adapter <ACE_Malloc <ACE_MMAP_MEMORY_POOL
                                             , TAO_SYNCH_MUTEX> > ALLOCATOR;

  PersistenceUpdater (void);
  virtual ~PersistenceUpdater (void);

  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Shared object finalizer
  virtual int fini (void);

  virtual int svc (void);

  // Request an image refresh to be sent to
  //  the specified callback (asynchronously).
  virtual void requestImage (void);

  // Add entities to be persisted.
  virtual void add (const TopicData& topic);
  virtual void add (const ParticipantData& participant);
  virtual void add (const ActorData& actor);

  // Remove an entity (but not children) from persistence.
  virtual void remove (const ItemType& itemType, const IdType& id);

  // Persist updated Qos parameters for an entity.
  virtual void updateQos (const ItemType& itemType, const IdType& id
			 , const QosType& qos);

 private:
  int parse (int argc, ACE_TCHAR *argv[]);

  std::string persistence_file_;
  bool overwrite_;

  UpdateManager *um_;

  ALLOCATOR *allocator_;

  TopicIndex *topic_index_;
  ParticipantIndex *participant_index_;
  ActorIndex *actor_index_;
};

#endif // _PERSISTENCE_UPDATER_
