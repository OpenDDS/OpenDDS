#include "PersistenceUpdater.h"

#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/OS_NS_strings.h"
#include "ace/Svc_Handler.h"

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
    , overwrite_ (true)
{ }

PersistenceUpdater::~PersistenceUpdater (void)
{ }

void* createIndex (const std::string& tag, PersistenceUpdater::ALLOCATOR& allocator
                   , size_t size)
{
  void* index = 0;

  // This is the easy case since if we find hash table in the
  // memory-mapped file we know it's already initialized.
  if (allocator.find (tag.c_str(), index) == 0) {
    return index;
  }
  else
    {
      ACE_ALLOCATOR_RETURN (index, allocator.malloc (size), 0);

      if (allocator.bind (tag.c_str(), index) == -1) {
        allocator.free (index);
        index = 0;
      }
    }

  return index;
}


int
PersistenceUpdater::init (int argc, ACE_TCHAR *argv[])
{
  this->parse (argc, argv);

  ACE_MMAP_Memory_Pool::OPTIONS options (ACE_DEFAULT_BASE_ADDR);

  // Create the allocator with the appropriate options.  The name used
  // for  the lock is the same as one used for the file.
  ACE_NEW_RETURN (allocator_,
                  ALLOCATOR (persistence_file_.c_str(),
                             persistence_file_.c_str(),
                             &options),
                  -1);

  char* index = 0;
  std::string topic_tag ("TopicIndex");
  std::string participant_tag ("ParticipantIndex");
  std::string actor_tag ("ActorIndex");

  index = (char*)createIndex (topic_tag, *allocator_, sizeof (TopicIndex));
  if (index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 1.\n"));
    return -1;
  }
  topic_index_ = new (index) TopicIndex (allocator_);

  index = 0;
  index = (char*)createIndex (participant_tag, *allocator_, sizeof (ParticipantIndex));
  if (index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 2.\n"));
    return -1;
  }
  participant_index_ = new (index) ParticipantIndex (allocator_);

  index = 0;
  index = (char*)createIndex (actor_tag, *allocator_, sizeof (ActorIndex));
  if (index == 0) {
    ACE_DEBUG ((LM_DEBUG, "Initial allocation/Bind failed 2.\n"));
    return -1;
  }
  actor_index_ = new (index) ActorIndex (allocator_);

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
}

void
PersistenceUpdater::add(const TopicData& topicData)
{
  // allocate memory for TopicData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc (sizeof(topicData)));

  // Initialize TopicData
  TopicData* topic_data = new (buffer) TopicData (topicData);

  IdType_ExtId ext (topicData.topicId);
  // bind TopicData with the topicId
  if (topic_index_->bind (ext, topic_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::add(const ParticipantData& participantData)
{
  // allocate memory for ParticipantData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc (sizeof(participantData)));

  // Initialize ParticipantData
  ParticipantData* participant_data = new (buffer) ParticipantData (participantData);

  IdType_ExtId ext (participantData.participantId);
  // bind ParticipantData with the participantId
  if (participant_index_->bind (ext, participant_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::add(const ActorData& actorData)
{
  // allocate memory for ActorData
  void* buffer;
  ACE_ALLOCATOR (buffer, allocator_->malloc (sizeof(actorData)));

  if (buffer == 0) {
    return;
  }

  // Initialize ActorData
  ActorData* actor_data = new (buffer) ActorData (actorData);

  IdType_ExtId ext (actorData.actorId);
  // bind ActorData with the actorId
  if (actor_index_->bind (ext, actor_data, allocator_) != 0)
    {
      allocator_->free ((void *) buffer);
      return;
    }
}

void
PersistenceUpdater::remove(const ItemType& itemType , const IdType& idType)
{
  IdType_ExtId ext (idType);

  switch (itemType)
    {
    case Topic:
      {
        TopicData* topicData = 0;
        if (topic_index_->unbind (ext, topicData, allocator_) == 0) {
          allocator_->free ((void *) topicData);
        }
        else {
          ACE_ERROR ((LM_ERROR, "(%P|%t) Invalid removal Topic Id"));
        }
      }
      break;
    case Participant:
      {
        ParticipantData* participantData = 0;
        if (participant_index_->unbind (ext, participantData, allocator_) == 0) {
          allocator_->free ((void *) participantData);
        }
        else {
          ACE_ERROR ((LM_ERROR, "(%P|%t) Invalid removal Participant Id"));
        }
      }
      break;
    case Actor:
      {
        ActorData* actorData = 0;
        if (actor_index_->unbind (ext, actorData, allocator_) == 0) {
          allocator_->free ((void *) actorData);
        }
        else {
          ACE_ERROR ((LM_ERROR, "(%P|%t) Invalid removal Actor Id"));
        }
      }
      break;
    default:
      ACE_ERROR ((LM_ERROR, "(%P | %t) Unknown removal type"));
    }
}

void
PersistenceUpdater::updateQos(const ItemType& , const IdType&
                                      , const QosType& )
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
