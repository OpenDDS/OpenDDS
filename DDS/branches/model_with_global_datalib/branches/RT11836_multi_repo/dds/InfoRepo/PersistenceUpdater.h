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

#include "dds/DdsDcpsInfoUtilsC.h"

#include "ace/Task.h"
#include "ace/Hash_Map_With_Allocator_T.h"
#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"

#include <string>

// Forward declaration
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

public:
  typedef ACE_Allocator_Adapter <ACE_Malloc <ACE_MMAP_MEMORY_POOL
                                             , TAO_SYNCH_MUTEX> > ALLOCATOR;

  /// Persisted entity data structures
  typedef struct TopicStrt <QosSeq, ACE_CString> Topic;
  typedef struct ParticipantStrt <QosSeq> Participant;
  typedef struct ActorStrt <QosSeq, QosSeq, ACE_CString, BinSeq> RWActor;

  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, Topic*> TopicIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, Participant*> ParticipantIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, RWActor*> ActorIndex;

public:
  PersistenceUpdater (void);
  virtual ~PersistenceUpdater (void);

  /// Service object initialization
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// pure ACE_Task_Base methods
  virtual int fini (void);
  virtual int svc (void);

  /// Request an image refresh to be sent upstream.
  /// This is currently done synchronously.
  /// TBD: Move to an asynchronous model
  virtual void requestImage (void);

  /// Add entities to be persisted.
  virtual void add (const UpdateManager::DTopic& topic);
  virtual void add (const UpdateManager::DParticipant& participant);
  virtual void add (const UpdateManager::DActor& actor);

  /// Remove an entity (but not children) from persistence.
  virtual void remove (ItemType type, const IdType& id);

  /// Persist updated Qos parameters for an entity.
  virtual void updateQos (const ItemType& itemType, const IdType& id
			 , const ::QosSeq& qos);

 private:
  int parse (int argc, ACE_TCHAR *argv[]);

  ACE_TString persistence_file_;
  bool reset_;

  UpdateManager *um_;

  ALLOCATOR *allocator_;

  TopicIndex *topic_index_;
  ParticipantIndex *participant_index_;
  ActorIndex *actor_index_;
};


#endif // _PERSISTENCE_UPDATER_
