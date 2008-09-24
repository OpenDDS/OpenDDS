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

#include "inforepo_export.h"
#include "Updater.h"

#include "dds/DdsDcpsInfoUtilsC.h"

#include "ace/Task.h"
#include "ace/Hash_Map_With_Allocator_T.h"
#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"

#include <string>

namespace Update {

// Forward declaration
class Manager;

class OpenDDS_InfoRepoLib_Export PersistenceUpdater : public Updater, public ACE_Task_Base
{
 public:
  class IdType_ExtId {
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
  virtual void create( const UTopic& topic);
  virtual void create( const UParticipant& participant);
  virtual void create( const URActor& actor);
  virtual void create( const UWActor& actor);
  virtual void create( const OwnershipData& data);

  /// Persist updated Qos parameters for an entity.
  virtual void update( const IdPath& id, const ::DDS::DomainParticipantQos& qos);
  virtual void update( const IdPath& id, const ::DDS::TopicQos&             qos);
  virtual void update( const IdPath& id, const ::DDS::DataWriterQos&        qos);
  virtual void update( const IdPath& id, const ::DDS::PublisherQos&         qos);
  virtual void update( const IdPath& id, const ::DDS::DataReaderQos&        qos);
  virtual void update( const IdPath& id, const ::DDS::SubscriberQos&        qos);

  /// Remove an entity (but not children) from persistence.
  virtual void destroy( const IdPath& id, ItemType type, ActorType actor);

 private:
  int parse (int argc, ACE_TCHAR *argv[]);
  void storeUpdate( const ACE_Message_Block& data, BinSeq& storage);

  ACE_TString persistence_file_;
  bool reset_;

  Manager *um_;

  ALLOCATOR *allocator_;

  TopicIndex *topic_index_;
  ParticipantIndex *participant_index_;
  ActorIndex *actor_index_;
};

} // End of namespace Update

#endif // _PERSISTENCE_UPDATER_

