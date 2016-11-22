/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PERSISTENCE_UPDATER_H
#define PERSISTENCE_UPDATER_H

#include "inforepo_export.h"
#include "Updater.h"

#include "dds/DdsDcpsInfoUtilsC.h"

#include "ace/Task.h"
#include "ace/Hash_Map_With_Allocator_T.h"
#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace Update {

// Forward declaration
class Manager;

class OpenDDS_InfoRepoLib_Export PersistenceUpdater : public Updater, public ACE_Task_Base {
public:
  class IdType_ExtId {
  public:
    IdType_ExtId();

    IdType_ExtId(IdType id);

    IdType_ExtId(const IdType_ExtId& ext);

    void operator= (const IdType_ExtId& ext);

    bool operator== (const IdType_ExtId& ext) const;

    unsigned long hash() const;

  private:
    IdType id_;
  };

public:
  typedef ACE_Allocator_Adapter <ACE_Malloc <ACE_MMAP_MEMORY_POOL
  , TAO_SYNCH_MUTEX> > ALLOCATOR;

  /// Persisted entity data structures
  typedef struct TopicStrt <QosSeq, ACE_CString> Topic;
  typedef struct ParticipantStrt <QosSeq> Participant;
  typedef struct ActorStrt <QosSeq, QosSeq, ACE_CString,
                            BinSeq, ContentSubscriptionBin> RWActor;

  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, Topic*> TopicIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, Participant*> ParticipantIndex;
  typedef ACE_Hash_Map_With_Allocator<IdType_ExtId, RWActor*> ActorIndex;

public:
  PersistenceUpdater();
  virtual ~PersistenceUpdater();

  /// Service object initialization
  virtual int init(int argc, ACE_TCHAR *argv[]);

  /// pure ACE_Task_Base methods
  virtual int fini();
  virtual int svc();

  /// Request an image refresh to be sent upstream.
  /// This is currently done synchronously.
  /// TBD: Move to an asynchronous model
  virtual void requestImage();

  /// Add entities to be persisted.
  virtual void create(const UTopic& topic);
  virtual void create(const UParticipant& participant);
  virtual void create(const URActor& actor);
  virtual void create(const UWActor& actor);
  virtual void create(const OwnershipData& data);

  /// Persist updated Qos parameters for an entity.
  virtual void update(const IdPath& id, const DDS::DomainParticipantQos& qos);
  virtual void update(const IdPath& id, const DDS::TopicQos&             qos);
  virtual void update(const IdPath& id, const DDS::DataWriterQos&        qos);
  virtual void update(const IdPath& id, const DDS::PublisherQos&         qos);
  virtual void update(const IdPath& id, const DDS::DataReaderQos&        qos);
  virtual void update(const IdPath& id, const DDS::SubscriberQos&        qos);
  virtual void update(const IdPath& id, const DDS::StringSeq&     exprParams);

  /// Remove an entity (but not children) from persistence.
  virtual void destroy(const IdPath& id, ItemType type, ActorType actor);

private:
  int parse(int argc, ACE_TCHAR *argv[]);
  void storeUpdate(const ACE_Message_Block& data, BinSeq& storage);

  ACE_TString persistence_file_;
  bool reset_;

  Manager *um_;

  ALLOCATOR *allocator_;

  TopicIndex *topic_index_;
  ParticipantIndex *participant_index_;
  ActorIndex *actor_index_;
};

} // End of namespace Update

typedef Update::PersistenceUpdater PersistenceUpdaterSvc;

ACE_STATIC_SVC_DECLARE(PersistenceUpdaterSvc)

ACE_FACTORY_DECLARE(ACE_Local_Service, PersistenceUpdaterSvc)

class OpenDDS_InfoRepoLib_Export PersistenceUpdaterSvc_Loader {
public:
  static int init();
};

#if defined(ACE_HAS_BROKEN_STATIC_CONSTRUCTORS)

typedef int (*PersistenceUpdaterSvc_Loader)();

static UpdateManagerSvc_Loader load = &UpdateManagerSvc_Loader::init;

#else

static int load = PersistenceUpdaterSvc_Loader::init();

#endif /* ACE_HAS_BROKEN_STATIC_CONSTRUCTORS */

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* PERSISTENCE_UPDATER_H */
