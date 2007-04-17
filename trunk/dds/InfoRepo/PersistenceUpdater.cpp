#include "DcpsInfo_pch.h"

#include "PersistenceUpdater.h"
#include "UpdateManager.h"
#include "ArrDelAdapter.h"

#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/OS_NS_strings.h"
#include "ace/Svc_Handler.h"
#include "ace/Dynamic_Service.h"

#include <algorithm>

// Template specializations with custom constructors
// and cleanup methods
template<>
struct TopicStrt <QosSeq, ACE_CString>
{
  IdType domainId;
  IdType topicId; // Unique system-wide
  IdType participantId;
  ACE_CString name;
  ACE_CString dataType;
  QosSeq topicQos;

  TopicStrt (const UpdateManager::DTopic& topic
             , PersistenceUpdater::ALLOCATOR* allocator)
    : domainId (topic.domainId)
    , topicId (topic.topicId)
    , participantId (topic.participantId)
    , name (topic.name.c_str(), allocator)
    , dataType (topic.dataType.c_str(), allocator)
  {
    int qos_len = topic.topicQos.second.first;
    char* in_buf = topic.topicQos.second.second;

    void* out_buf;
    ACE_ALLOCATOR (out_buf, allocator->malloc (qos_len));
    ACE_OS::memcpy (out_buf, in_buf, qos_len);

    topicQos.first = TopicQos;
    topicQos.second.first = qos_len;
    topicQos.second.second = static_cast<char*>(out_buf);
  };

  void cleanup (PersistenceUpdater::ALLOCATOR* allocator)
  {
    allocator->free (topicQos.second.second);
  };
};

template<>
struct ParticipantStrt <QosSeq>
{
  IdType domainId;
  IdType participantId; // Unique system-wide
  QosSeq participantQos;

  ParticipantStrt (const IdType& dId, const IdType& pId
                   , const QosSeq& pQos)
    : domainId (dId), participantId (pId)
      , participantQos (pQos)
  {}

  ParticipantStrt (const UpdateManager::DParticipant& participant
                   , PersistenceUpdater::ALLOCATOR* allocator)
    : domainId (participant.domainId)
      , participantId (participant.participantId)
  {
    int qos_len = participant.participantQos.second.first;
    char* in_buf = participant.participantQos.second.second;

    void* out_buf;
    ACE_ALLOCATOR (out_buf, allocator->malloc (qos_len));
    ACE_OS::memcpy (out_buf, in_buf, qos_len);

    participantQos.first = ParticipantQos;
    participantQos.second.first = qos_len;
    participantQos.second.second = static_cast<char*>(out_buf);
  }

 void cleanup (PersistenceUpdater::ALLOCATOR* allocator)
  {
    allocator->free (participantQos.second.second);
  }
};

template<>
struct ActorStrt <QosSeq, QosSeq
                  , ACE_CString, BinSeq>
{
  IdType domainId;
  IdType actorId; // Unique system-wide
  IdType topicId;
  IdType participantId;
  ActorType type;
  ACE_CString callback;
  QosSeq pubsubQos;
  QosSeq drdwQos;
  BinSeq transportInterfaceInfo;

  ActorStrt (const UpdateManager::DActor& actor
             , PersistenceUpdater::ALLOCATOR* allocator)
    : domainId (actor.domainId), actorId (actor.actorId)
    , topicId (actor.topicId)
    , participantId (actor.participantId), type (actor.type)
    , callback (actor.callback.c_str(), allocator)
    {
      int qos_len = actor.pubsubQos.second.first;
      char* in_buf = actor.pubsubQos.second.second;

      void* out_buf;
      ACE_ALLOCATOR (out_buf, allocator->malloc (qos_len));
      ACE_OS::memcpy (out_buf, in_buf, qos_len);

      pubsubQos.first = actor.pubsubQos.first;
      pubsubQos.second.first = qos_len;
      pubsubQos.second.second = static_cast<char*>(out_buf);


      qos_len = actor.drdwQos.second.first;
      in_buf = actor.drdwQos.second.second;
      ACE_ALLOCATOR (out_buf, allocator->malloc (qos_len));
      ACE_OS::memcpy (out_buf, in_buf, qos_len);
      drdwQos.first = actor.drdwQos.first;
      drdwQos.second.first = qos_len;
      drdwQos.second.second = static_cast<char*>(out_buf);


      qos_len = actor.transportInterfaceInfo.first;
      in_buf = actor.transportInterfaceInfo.second;
      ACE_ALLOCATOR (out_buf, allocator->malloc (qos_len));
      ACE_OS::memcpy (out_buf, in_buf, qos_len);
      transportInterfaceInfo.first = qos_len;
      transportInterfaceInfo.second = static_cast<char*>(out_buf);
    };


  void cleanup (PersistenceUpdater::ALLOCATOR* allocator)
  {
    allocator->free (pubsubQos.second.second);
    allocator->free (drdwQos.second.second);
    allocator->free (transportInterfaceInfo.second);
  };
};

PersistenceUpdater::IdType_ExtId::IdType_ExtId ()
{}

PersistenceUpdater::IdType_ExtId::IdType_ExtId (IdType id)
  : id_ (id)
{}

PersistenceUpdater::IdType_ExtId::IdType_ExtId (const IdType_ExtId& ext)
  : id_ (ext.id_)
{}

void
PersistenceUpdater::IdType_ExtId::operator= (const IdType_ExtId& ext)
{
  id_ = ext.id_;
}

bool
PersistenceUpdater::IdType_ExtId::operator== (const IdType_ExtId& ext) const
{
  return (id_ == ext.id_);
}

unsigned long
PersistenceUpdater::IdType_ExtId::hash (void) const
{
  return (unsigned long) id_;
};




PersistenceUpdater::PersistenceUpdater (void)
  : persistence_file_ ("InforepoPersist")
    , reset_ (false)
    , um_ (0)
{ }

PersistenceUpdater::~PersistenceUpdater (void)
{
}

// utility functions
void* createIndex (const std::string& tag
                   , PersistenceUpdater::ALLOCATOR& allocator
                   , size_t size, bool& exists)
{
  void* index = 0;

  // This is the easy case since if we find hash table in the
  // memory-mapped file we know it's already initialized.
  if (allocator.find (tag.c_str(), index) == 0) {
    exists = true;
    return index;
  }
  else
    {
      exists = false;

      ACE_ALLOCATOR_RETURN (index, allocator.malloc (size), 0);

      if (allocator.bind (tag.c_str(), index) == -1) {
        allocator.free (index);
        index = 0;
      }
    }

  return index;
}

template<typename I> void
index_cleanup (I* index
               , PersistenceUpdater::ALLOCATOR* allocator)
{
  for (typename I::ITERATOR iter = index->begin()
         ; iter != index->end(); )
    {
      typename I::ITERATOR current_iter = iter;
      iter++;

      if (index->unbind ((*current_iter).ext_id_, allocator) != 0) {
        ACE_ERROR ((LM_ERROR, "Index unbind failed.\n"));
      }
    }
}

int
PersistenceUpdater::init (int argc, ACE_TCHAR *argv[])
{
  // discover the UpdateManager
  um_ = ACE_Dynamic_Service<UpdateManager>::instance
    ("UpdateManager");

  if (um_ == 0) {
    ACE_ERROR ((LM_ERROR, "PersistenceUpdater initialization failed. "
                "No UpdateManager discovered.\n"));
    return -1;
  }

  this->parse (argc, argv);

  ACE_MMAP_Memory_Pool::OPTIONS options (ACE_DEFAULT_BASE_ADDR);

  // Create the allocator with the appropriate options.  The name used
  // for  the lock is the same as one used for the file.
  ACE_NEW_RETURN (allocator_,
                  ALLOCATOR (persistence_file_.c_str(),
                             persistence_file_.c_str(),
                             &options),
                  -1);

  std::string topic_tag ("TopicIndex");
  std::string participant_tag ("ParticipantIndex");
  std::string actor_tag ("ActorIndex");
  bool exists = false, ex = false;

  char* topic_index = (char*)createIndex (topic_tag, *allocator_
                                          , sizeof (TopicIndex), ex);
  if (topic_index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 1.\n"));
    return -1;
  }
  exists = exists || ex;

  char* participant_index = (char*)createIndex (participant_tag, *allocator_
                                                , sizeof (ParticipantIndex), ex);
  if (participant_index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 2.\n"));
    return -1;
  }
  exists = exists || ex;

  char* actor_index = (char*)createIndex (actor_tag, *allocator_
                                          , sizeof (ActorIndex), ex);
  if (actor_index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 2.\n"));
    return -1;
  }
  exists = exists || ex;

  if (exists)
    {
      topic_index_ = reinterpret_cast<TopicIndex*> (topic_index);
      participant_index_ = reinterpret_cast<ParticipantIndex*> (participant_index);
      actor_index_ = reinterpret_cast<ActorIndex*> (actor_index);

      if (!(topic_index_ && participant_index_ && actor_index_)) {
        ACE_ERROR ((LM_DEBUG, "Unable to narrow persistent indexes.\n"));
        return -1;
      }
    }
  else
    {
      topic_index_ = new (topic_index) TopicIndex (allocator_);
      participant_index_ = new (participant_index) ParticipantIndex (allocator_);
      actor_index_ = new (actor_index) ActorIndex (allocator_);
    }

  if (reset_)
    {
      index_cleanup (topic_index_, allocator_);
      index_cleanup (participant_index_, allocator_);
      index_cleanup (actor_index_, allocator_);
    }

  // lastly register the callback
  um_->add (static_cast<Updater*>(this));

  return 0;
}

int
PersistenceUpdater::parse (int argc, ACE_TCHAR *argv[])
{
  for (ssize_t count = 0; count < argc; count++)
    {
      if (ACE_OS::strcasecmp (argv[count], "-file") == 0)
        {
          if ((count + 1) < argc) {
            persistence_file_ = argv[count+1];
            count++;
          }
        }
      else if (ACE_OS::strcasecmp (argv[count], "-reset") == 0)
        {
          if ((count + 1) < argc) {
            int val = ACE_OS::atoi (argv[count+1]);
            reset_ = true;

            if (val == 0) {
              reset_ = false;
            }
            count++;
          }
        }
      else {
        ACE_DEBUG ((LM_DEBUG, "Unknown option %s\n"
                    , argv[count]));
        return -1;
      }
    }

  return 0;
}

int
PersistenceUpdater::fini (void)
{
  return 0;
}

int
PersistenceUpdater::svc (void)
{
  return 0;
}

void
PersistenceUpdater::requestImage (void)
{
  if (um_ == NULL) {
    return;
  }

  UpdateManager::DImage image;

  // Allocate space to hold the QOS sequences.
  std::vector<ArrDelAdapter<char> > qos_sequences;

  for (TopicIndex::ITERATOR iter = topic_index_->begin();
       iter != topic_index_->end(); iter++)
    {
      const PersistenceUpdater::Topic* topic = (*iter).int_id_;

      size_t qos_len = topic->topicQos.second.first;
      char *buf;
      ACE_NEW_NORETURN (buf, char[qos_len]);
      qos_sequences.push_back (ArrDelAdapter<char>(buf));
      if (buf == 0) {
        ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
        return;
      }
      ACE_OS::memcpy (buf, topic->topicQos.second.second, qos_len);

      BinSeq in_seq (qos_len, buf);
      QosSeq qos (TopicQos, in_seq);
      UpdateManager::DTopic dTopic (topic->domainId, topic->topicId
                                    , topic->participantId, topic->name.c_str()
                                    , topic->dataType.c_str(), qos);
      image.topics.push_back (dTopic);
    }

  for (ParticipantIndex::ITERATOR iter = participant_index_->begin();
       iter != participant_index_->end(); iter++)
    {
      const PersistenceUpdater::Participant* participant
        = (*iter).int_id_;

      size_t qos_len = participant->participantQos.second.first;
      char *buf;
      ACE_NEW_NORETURN (buf, char[qos_len]);
      qos_sequences.push_back (ArrDelAdapter<char>(buf));
      if (buf == 0) {
        ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
        return;
      }
      ACE_OS::memcpy (buf, participant->participantQos.second.second, qos_len);

      BinSeq in_seq (qos_len, buf);
      QosSeq qos (ParticipantQos, in_seq);
      UpdateManager::DParticipant dparticipant (participant->domainId
                                                , participant->participantId
                                                , qos);
      image.participants.push_back (dparticipant);
    }

  for (ActorIndex::ITERATOR iter = actor_index_->begin();
       iter != actor_index_->end(); iter++)
    {
      const PersistenceUpdater::RWActor* actor = (*iter).int_id_;

      size_t qos_len = actor->pubsubQos.second.first;
      char *buf;
      ACE_NEW_NORETURN (buf, char[qos_len]);
      qos_sequences.push_back (ArrDelAdapter<char>(buf));
      if (buf == 0) {
        ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
        return;
      }
      ACE_OS::memcpy (buf, actor->pubsubQos.second.second, qos_len);

      BinSeq in_pubsub_seq (qos_len, buf);
      QosSeq pubsub_qos (actor->pubsubQos.first, in_pubsub_seq);



      qos_len = actor->drdwQos.second.first;
      ACE_NEW_NORETURN (buf, char[qos_len]);
      qos_sequences.push_back (ArrDelAdapter<char>(buf));
      if (buf == 0) {
        ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
        return;
      }
      ACE_OS::memcpy (buf, actor->drdwQos.second.second, qos_len);

      BinSeq in_drdw_seq (qos_len, buf);
      QosSeq drdw_qos (actor->drdwQos.first, in_drdw_seq);



      qos_len = actor->transportInterfaceInfo.first;
      ACE_NEW_NORETURN (buf, char[qos_len]);
      qos_sequences.push_back (ArrDelAdapter<char>(buf));
      if (buf == 0) {
        ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
        return;
      }
      ACE_OS::memcpy (buf, actor->transportInterfaceInfo.second, qos_len);

      BinSeq in_transport_seq (qos_len, buf);


      UpdateManager::DActor dActor
        (actor->domainId, actor->actorId, actor->topicId
         , actor->participantId
         , actor->type, actor->callback.c_str()
         , pubsub_qos, drdw_qos, in_transport_seq);
      image.actors.push_back (dActor);
    }

  um_->pushImage (image);
}

void
PersistenceUpdater::add(const UpdateManager::DTopic& topic)
{
  // allocate memory for TopicData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc
                 (sizeof(PersistenceUpdater::Topic)));

  // Initialize TopicData
  PersistenceUpdater::Topic* topic_data
    = new (buffer) PersistenceUpdater::Topic (topic, allocator_);
  ACE_DEBUG ((LM_DEBUG, "add> Topic id: %d\n", topic_data->topicId));

  IdType_ExtId ext (topic.topicId);
  // bind TopicData with the topicId
  if (topic_index_->bind (ext, topic_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::add(const UpdateManager::DParticipant& participant)
{

  // allocate memory for ParticipantData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc
                 (sizeof(PersistenceUpdater::Participant)));

  // Initialize ParticipantData
  PersistenceUpdater::Participant* participant_data
    = new (buffer) PersistenceUpdater::Participant (participant
                                                    , allocator_);

  IdType_ExtId ext (participant.participantId);
  // bind ParticipantData with the participantId
  if (participant_index_->bind (ext, participant_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::add(const UpdateManager::DActor& actor)
{

  // allocate memory for ActorData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc
                 (sizeof(PersistenceUpdater::RWActor)));

  // Initialize ActorData
  PersistenceUpdater::RWActor* actor_data =
    new (buffer) PersistenceUpdater::RWActor (actor
                                            , allocator_);

  IdType_ExtId ext (actor.actorId);
  // bind ActorData with the actorId
  if (actor_index_->bind (ext, actor_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::remove (ItemType type, const IdType& idType)
{
  IdType_ExtId ext (idType);
  PersistenceUpdater::Topic* topic = 0;
  PersistenceUpdater::Participant* participant = 0;
  PersistenceUpdater::RWActor* actor = 0;

  switch (type)
    {
    case ::Topic:
      if (topic_index_->unbind (ext, topic, allocator_) == 0)
        {
          topic->cleanup (allocator_);
          allocator_->free ((void *) topic);
          ACE_DEBUG ((LM_DEBUG, "Removed persistent topic: %d\n"
                      , idType));
        }
      break;
    case ::Participant:
      if (participant_index_->unbind (ext, participant, allocator_) == 0) {
        participant->cleanup (allocator_);
        allocator_->free ((void *) participant);
        ACE_DEBUG ((LM_DEBUG, "Removed persistent participant: %d\n"
                    , idType));
      }
      break;
    case ::Actor:
      if (actor_index_->unbind (ext, actor, allocator_) == 0) {
        actor->cleanup (allocator_);
        allocator_->free ((void *) actor);
        ACE_DEBUG ((LM_DEBUG, "Removed persistent actor: %d\n"
                    , idType));
      }
      break;
    default:
      ACE_ERROR ((LM_ERROR, "(%P | %t) Unknown entity: %d\n", idType));
    }
}

void
PersistenceUpdater::updateQos(const ItemType& , const IdType&
                                      , const ::QosSeq& )
{
  // TBD
}

// from the "ACE Programmers Guide (P. 424)

ACE_FACTORY_DEFINE (ACE_Local_Service, PersistenceUpdater)

ACE_STATIC_SVC_DEFINE (PersistenceUpdater,
                         ACE_TEXT ("PersistenceUpdater_Static_Service"),
                         ACE_SVC_OBJ_T,
                         &ACE_SVC_NAME (PersistenceUpdater),
                         ACE_Service_Type::DELETE_THIS |
                         ACE_Service_Type::DELETE_OBJ,
                         0)

ACE_STATIC_SVC_REQUIRE (PersistenceUpdater)
