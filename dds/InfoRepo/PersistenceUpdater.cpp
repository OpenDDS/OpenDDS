/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include "PersistenceUpdater.h"
#include "UpdateManager.h"
#include "ArrDelAdapter.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/GuidUtils.h"

#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/OS_NS_strings.h"
#include "ace/Svc_Handler.h"
#include "ace/Dynamic_Service.h"

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {
  void assign(Update::BinSeq& to, const Update::BinSeq& from,
              Update::PersistenceUpdater::ALLOCATOR* allocator)
  {
    const size_t len = from.first;
    void* out_buf;
    ACE_ALLOCATOR(out_buf, allocator->malloc(len));
    ACE_OS::memcpy(out_buf, from.second, len);
    to = std::make_pair(len, static_cast<char*>(out_buf));
  }

  void assign(ACE_CString& to, const char* from,
              Update::PersistenceUpdater::ALLOCATOR* allocator)
  {
    const size_t len = ACE_OS::strlen (from) + 1;
    void* out_buf;
    ACE_ALLOCATOR(out_buf, allocator->malloc(len));
    ACE_OS::memcpy(out_buf, from, len);
    to.set(static_cast<char*>(out_buf), len - 1, false);
  }
}

namespace Update {

// Template specializations with custom constructors
// and cleanup methods
template<>
struct TopicStrt<QosSeq, ACE_CString> {
  DDS::DomainId_t   domainId;
  IdType            topicId;
  IdType            participantId;
  ACE_CString       name;
  ACE_CString       dataType;
  QosSeq            topicQos;

  TopicStrt(const DTopic& topic,
            PersistenceUpdater::ALLOCATOR* allocator)
    : domainId(topic.domainId),
      topicId(topic.topicId),
      participantId(topic.participantId)
  {
    assign(name, topic.name.c_str(), allocator);
    assign(dataType, topic.dataType.c_str(), allocator);

    topicQos.first = TopicQos;
    assign(topicQos.second, topic.topicQos.second, allocator);
  }

  void cleanup(PersistenceUpdater::ALLOCATOR* allocator)
  {
    if (name.length() > 0)
    {
      char* strMemory = const_cast<char*>(name.fast_rep());
      name.fast_clear();
      allocator->free(strMemory);
    }
    if (dataType.length() > 0)
    {
      char* strMemory = const_cast<char*>(dataType.fast_rep());
      dataType.fast_clear();
      allocator->free(strMemory);
    }

    allocator->free(topicQos.second.second);
  }
};

template<>
struct ParticipantStrt<QosSeq> {
  DDS::DomainId_t   domainId;
  long              owner;
  IdType            participantId;
  QosSeq            participantQos;

  ParticipantStrt(const DDS::DomainId_t& dId,
                  long                   own,
                  const IdType&          pId,
                  const QosSeq&          pQos)
    : domainId(dId),
      owner(own),
      participantId(pId),
      participantQos(pQos) {}

  ParticipantStrt(const DParticipant& participant,
                  PersistenceUpdater::ALLOCATOR* allocator)
    : domainId(participant.domainId),
      owner(participant.owner),
      participantId(participant.participantId)
  {
    participantQos.first = ParticipantQos;
    assign(participantQos.second, participant.participantQos.second, allocator);
  }

  void cleanup(PersistenceUpdater::ALLOCATOR* allocator)
  {
    allocator->free(participantQos.second.second);
  }
};

template<>
struct ActorStrt<QosSeq, QosSeq,
                 ACE_CString, BinSeq, ContentSubscriptionBin> {
  DDS::DomainId_t   domainId;
  IdType            actorId;
  IdType            topicId;
  IdType            participantId;
  ActorType         type;
  ACE_CString       callback;
  QosSeq            pubsubQos;
  QosSeq            drdwQos;
  BinSeq            transportInterfaceInfo;
  ContentSubscriptionBin contentSubscriptionProfile;

  ActorStrt(const DActor& actor,
            PersistenceUpdater::ALLOCATOR* allocator)
    : domainId(actor.domainId),
      actorId(actor.actorId),
      topicId(actor.topicId),
      participantId(actor.participantId), type(actor.type)
  {
    assign(callback, actor.callback.c_str(), allocator);

    pubsubQos.first = actor.pubsubQos.first;
    assign(pubsubQos.second, actor.pubsubQos.second, allocator);

    drdwQos.first = actor.drdwQos.first;
    assign(drdwQos.second, actor.drdwQos.second, allocator);

    assign(transportInterfaceInfo, actor.transportInterfaceInfo, allocator);

    contentSubscriptionProfile.filterClassName =
      ACE_CString(actor.contentSubscriptionProfile.filterClassName.c_str(),
                  allocator);
    contentSubscriptionProfile.filterExpr =
      ACE_CString(actor.contentSubscriptionProfile.filterExpr.c_str(),
                  allocator);
    assign(contentSubscriptionProfile.exprParams,
      actor.contentSubscriptionProfile.exprParams, allocator);
  }

  void cleanup(PersistenceUpdater::ALLOCATOR* allocator)
  {
    if (callback.length() > 0)
    {
      char* strMemory = const_cast<char*>(callback.fast_rep());
      callback.fast_clear();
      allocator->free(strMemory);
    }

    allocator->free(pubsubQos.second.second);
    allocator->free(drdwQos.second.second);
    allocator->free(transportInterfaceInfo.second);
    allocator->free(contentSubscriptionProfile.exprParams.second);
  }
};

PersistenceUpdater::IdType_ExtId::IdType_ExtId()
  : id_(OpenDDS::DCPS::GUID_UNKNOWN)
{}

PersistenceUpdater::IdType_ExtId::IdType_ExtId(IdType id)
  : id_(id)
{}

PersistenceUpdater::IdType_ExtId::IdType_ExtId(const IdType_ExtId& ext)
  : id_(ext.id_)
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
PersistenceUpdater::IdType_ExtId::hash() const
{
  return OpenDDS::DCPS::RepoIdConverter(id_).checksum();
};

PersistenceUpdater::PersistenceUpdater()
  : persistence_file_(ACE_TEXT("InforepoPersist"))
  , reset_(false)
  , um_(0)
  , allocator_(0)
  , topic_index_(0)
  , participant_index_(0)
  , actor_index_(0)

{}

PersistenceUpdater::~PersistenceUpdater()
{}

// utility functions
void* createIndex(const std::string& tag
                  , PersistenceUpdater::ALLOCATOR& allocator
                  , size_t size, bool& exists)
{
  void* index = 0;

  // This is the easy case since if we find hash table in the
  // memory-mapped file we know it's already initialized.
  if (allocator.find(tag.c_str(), index) == 0) {
    exists = true;
    return index;

  } else {
    exists = false;

    ACE_ALLOCATOR_RETURN(index, allocator.malloc(size), 0);

    if (allocator.bind(tag.c_str(), index) == -1) {
      allocator.free(index);
      index = 0;
    }
  }

  return index;
}

template<typename I> void
index_cleanup(I* index
              , PersistenceUpdater::ALLOCATOR* allocator)
{
  for (typename I::ITERATOR iter = index->begin()
                                   ; iter != index->end();) {
    typename I::ITERATOR current_iter = iter;
    iter++;

    if (index->unbind((*current_iter).ext_id_, allocator) != 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Index unbind failed.\n")));
    }
  }
}

int
PersistenceUpdater::init(int argc, ACE_TCHAR *argv[])
{
  // discover the UpdateManager
  um_ = ACE_Dynamic_Service<Update::Manager>::instance
        ("UpdateManagerSvc");

  if (um_ == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("PersistenceUpdater initialization failed. ")
               ACE_TEXT("No UpdateManager discovered.\n")));
    return -1;
  }

  this->parse(argc, argv);

#if defined ACE_HAS_MAC_OSX && defined __x86_64__ && __x86_64__
  ACE_MMAP_Memory_Pool::OPTIONS options((void*)0x200000000);
#else
  ACE_MMAP_Memory_Pool::OPTIONS options(ACE_DEFAULT_BASE_ADDR);
#endif

  // Create the allocator with the appropriate options.  The name used
  // for  the lock is the same as one used for the file.
  ACE_NEW_RETURN(allocator_,
                 ALLOCATOR(persistence_file_.c_str(),
                           persistence_file_.c_str(),
                           &options),
                 -1);

  std::string topic_tag("TopicIndex");
  std::string participant_tag("ParticipantIndex");
  std::string actor_tag("ActorIndex");
  bool exists = false, ex = false;

  char* topic_index = (char*)createIndex(topic_tag, *allocator_
                                         , sizeof(TopicIndex), ex);

  if (topic_index == 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Initial allocation/Bind failed 1.\n")));
    return -1;
  }

  exists = exists || ex;

  char* participant_index = (char*)createIndex(participant_tag, *allocator_
                                               , sizeof(ParticipantIndex), ex);

  if (participant_index == 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Initial allocation/Bind failed 2.\n")));
    return -1;
  }

  exists = exists || ex;

  char* actor_index = (char*)createIndex(actor_tag, *allocator_
                                         , sizeof(ActorIndex), ex);

  if (actor_index == 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Initial allocation/Bind failed 2.\n")));
    return -1;
  }

  exists = exists || ex;

  if (exists) {
    topic_index_ = reinterpret_cast<TopicIndex*>(topic_index);
    participant_index_ = reinterpret_cast<ParticipantIndex*>(participant_index);
    actor_index_ = reinterpret_cast<ActorIndex*>(actor_index);

    if (!(topic_index_ && participant_index_ && actor_index_)) {
      ACE_ERROR((LM_DEBUG, ACE_TEXT("Unable to narrow persistent indexes.\n")));
      return -1;
    }

  } else {
    topic_index_ = new(topic_index) TopicIndex(allocator_);
    participant_index_ = new(participant_index) ParticipantIndex(allocator_);
    actor_index_ = new(actor_index) ActorIndex(allocator_);
  }

  if (reset_) {
    index_cleanup(topic_index_, allocator_);
    index_cleanup(participant_index_, allocator_);
    index_cleanup(actor_index_, allocator_);
  }

  // lastly register the callback
  um_->add(this);

  return 0;
}

int
PersistenceUpdater::parse(int argc, ACE_TCHAR *argv[])
{
  for (ssize_t count = 0; count < argc; count++) {
    if (ACE_OS::strcasecmp(argv[count], ACE_TEXT("-file")) == 0) {
      if ((count + 1) < argc) {
        persistence_file_ = argv[count+1];
        count++;
      }

    } else if (ACE_OS::strcasecmp(argv[count], ACE_TEXT("-reset")) == 0) {
      if ((count + 1) < argc) {
        int val = ACE_OS::atoi(argv[count+1]);
        reset_ = true;

        if (val == 0) {
          reset_ = false;
        }

        count++;
      }

    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("Unknown option %s\n")
                 , argv[count]));
      return -1;
    }
  }

  return 0;
}

int
PersistenceUpdater::fini()
{
  return 0;
}

int
PersistenceUpdater::svc()
{
  return 0;
}

void
PersistenceUpdater::requestImage()
{
  if (um_ == NULL) {
    return;
  }

  DImage image;

  // Allocate space to hold the QOS sequences.
  std::vector<ArrDelAdapter<char> > qos_sequences;

  for (ParticipantIndex::ITERATOR iter = participant_index_->begin();
       iter != participant_index_->end(); iter++) {
    const PersistenceUpdater::Participant* participant
    = (*iter).int_id_;

    size_t qos_len = participant->participantQos.second.first;
    char *buf;
    ACE_NEW_NORETURN(buf, char[qos_len]);
    qos_sequences.push_back(ArrDelAdapter<char>(buf));

    if (buf == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("PersistenceUpdater::requestImage(): allocation failed.\n")));
      return;
    }

    ACE_OS::memcpy(buf, participant->participantQos.second.second, qos_len);

    BinSeq in_seq(qos_len, buf);
    QosSeq qos(ParticipantQos, in_seq);
    DParticipant dparticipant(participant->domainId
                              , participant->owner
                              , participant->participantId
                              , qos);
    image.participants.push_back(dparticipant);
  }

  for (TopicIndex::ITERATOR iter = topic_index_->begin();
       iter != topic_index_->end(); iter++) {
    const PersistenceUpdater::Topic* topic = (*iter).int_id_;

    size_t qos_len = topic->topicQos.second.first;
    char *buf;
    ACE_NEW_NORETURN(buf, char[qos_len]);
    qos_sequences.push_back(ArrDelAdapter<char>(buf));

    if (buf == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("PersistenceUpdater::requestImage(): allocation failed.\n")));
      return;
    }

    ACE_OS::memcpy(buf, topic->topicQos.second.second, qos_len);

    BinSeq in_seq(qos_len, buf);
    QosSeq qos(TopicQos, in_seq);
    DTopic dTopic(topic->domainId, topic->topicId
                  , topic->participantId, topic->name.c_str()
                  , topic->dataType.c_str(), qos);
    image.topics.push_back(dTopic);
  }

  for (ActorIndex::ITERATOR iter = actor_index_->begin();
       iter != actor_index_->end(); iter++) {
    const PersistenceUpdater::RWActor* actor = (*iter).int_id_;

    size_t qos_len = actor->pubsubQos.second.first;
    char *buf;
    ACE_NEW_NORETURN(buf, char[qos_len]);
    qos_sequences.push_back(ArrDelAdapter<char>(buf));

    if (buf == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("PersistenceUpdater::requestImage(): allocation failed.\n")));
      return;
    }

    ACE_OS::memcpy(buf, actor->pubsubQos.second.second, qos_len);

    BinSeq in_pubsub_seq(qos_len, buf);
    QosSeq pubsub_qos(actor->pubsubQos.first, in_pubsub_seq);

    qos_len = actor->drdwQos.second.first;
    ACE_NEW_NORETURN(buf, char[qos_len]);
    qos_sequences.push_back(ArrDelAdapter<char>(buf));

    if (buf == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("PersistenceUpdater::requestImage(): allocation failed.\n")));
      return;
    }

    ACE_OS::memcpy(buf, actor->drdwQos.second.second, qos_len);

    BinSeq in_drdw_seq(qos_len, buf);
    QosSeq drdw_qos(actor->drdwQos.first, in_drdw_seq);

    qos_len = actor->transportInterfaceInfo.first;
    ACE_NEW_NORETURN(buf, char[qos_len]);
    qos_sequences.push_back(ArrDelAdapter<char>(buf));

    if (buf == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("PersistenceUpdater::requestImage(): allocation failed.\n")));
      return;
    }

    ACE_OS::memcpy(buf, actor->transportInterfaceInfo.second, qos_len);

    BinSeq in_transport_seq(qos_len, buf);

    ContentSubscriptionBin in_csp_bin;
    if (actor->type == DataReader) {
      in_csp_bin.filterClassName = actor->contentSubscriptionProfile.filterClassName;
      in_csp_bin.filterExpr = actor->contentSubscriptionProfile.filterExpr;
      BinSeq& params = in_csp_bin.exprParams;
      ACE_NEW_NORETURN(params.second, char[params.first]);
      if (params.second == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("PersistenceUpdater::requestImage(): allocation ")
                   ACE_TEXT("failed.\n")));
        return;
      }
      qos_sequences.push_back(ArrDelAdapter<char>(params.second));
      ACE_OS::memcpy(params.second,
        actor->contentSubscriptionProfile.exprParams.second, params.first);
    }

    DActor dActor(actor->domainId, actor->actorId, actor->topicId
                  , actor->participantId
                  , actor->type, actor->callback.c_str()
                  , pubsub_qos, drdw_qos, in_transport_seq, in_csp_bin);
    image.actors.push_back(dActor);
  }

  um_->pushImage(image);
}

void
PersistenceUpdater::create(const UTopic& topic)
{
  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << topic.topicQos;
  ACE_Message_Block dst;
  ACE_CDR::consolidate(&dst, outCdr.begin());

  size_t len = dst.length();
  char *buf;
  ACE_NEW_NORETURN(buf, char[len]);
  ArrDelAdapter<char> guard(buf);

  if (buf == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("PersistenceUpdater::create( UTopic): allocation failed.\n")));
    return;
  }

  ACE_OS::memcpy(buf, dst.base(), len);

  BinSeq qos_bin(len, buf);

  QosSeq p(TopicQos, qos_bin);
  DTopic topic_data(topic.domainId, topic.topicId, topic.participantId
                    , topic.name.c_str(), topic.dataType.c_str(), p);

  // allocate memory for TopicData
  void* buffer;
  ACE_ALLOCATOR(buffer, allocator_->malloc
                (sizeof(PersistenceUpdater::Topic)));

  // Initialize TopicData
  PersistenceUpdater::Topic* persistent_data
  = new(buffer) PersistenceUpdater::Topic(topic_data, allocator_);

  IdType_ExtId ext(topic_data.topicId);

  // bind TopicData with the topicId
  if (topic_index_->bind(ext, persistent_data, allocator_) != 0) {
    allocator_->free((void *) buffer);
    return;
  }
}

void
PersistenceUpdater::create(const UParticipant& participant)
{
  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << participant.participantQos;
  ACE_Message_Block dst;
  ACE_CDR::consolidate(&dst, outCdr.begin());

  size_t len = dst.length();
  char *buf;
  ACE_NEW_NORETURN(buf, char[len]);
  ArrDelAdapter<char> guard(buf);

  if (buf == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("PersistenceUpdater::create( UParticipant): allocation failed.\n")));
    return;
  }

  ACE_OS::memcpy(buf, dst.base(), len);

  BinSeq qos_bin(len, buf);

  QosSeq p(ParticipantQos, qos_bin);
  DParticipant participant_data
  (participant.domainId, participant.owner, participant.participantId, p);

  // allocate memory for ParticipantData
  void* buffer;
  ACE_ALLOCATOR(buffer, allocator_->malloc
                (sizeof(PersistenceUpdater::Participant)));

  // Initialize ParticipantData
  PersistenceUpdater::Participant* persistent_data
  = new(buffer) PersistenceUpdater::Participant(participant_data
                                                , allocator_);

  IdType_ExtId ext(participant_data.participantId);

  // bind ParticipantData with the participantId
  if (participant_index_->bind(ext, persistent_data, allocator_) != 0) {
    allocator_->free((void *) buffer);
    return;
  }
}

void
PersistenceUpdater::create(const URActor& actor)
{
  TAO_OutputCDR outCdr;
  outCdr << actor.pubsubQos;
  ACE_Message_Block dst;
  ACE_CDR::consolidate(&dst, outCdr.begin());

  size_t len = dst.length();
  char *buf = new (std::nothrow) char[len];

  if (buf == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( subscription): allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard(buf);

  ACE_OS::memcpy(buf, dst.base(), len);

  BinSeq pubsub_qos_bin(len, buf);
  QosSeq pubsub_qos(SubscriberQos, pubsub_qos_bin);

  outCdr.reset();
  outCdr << actor.drdwQos;
  ACE_Message_Block dst2;
  ACE_CDR::consolidate(&dst2, outCdr.begin());

  len = dst2.length();
  char *buf2 = new (std::nothrow) char[len];

  if (buf2 == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( subscription): allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard2(buf2);

  ACE_OS::memcpy(buf2, dst2.base(), len);

  BinSeq dwdr_qos_bin(len, buf2);
  QosSeq dwdr_qos(DataReaderQos, dwdr_qos_bin);

  outCdr.reset();
  outCdr << actor.transportInterfaceInfo;
  ACE_Message_Block dst3;
  ACE_CDR::consolidate(&dst3, outCdr.begin());

  len = dst3.length();
  char *buf3 = new (std::nothrow) char[len];

  if (buf3 == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( subscription) allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard3(buf3);

  ACE_OS::memcpy(buf3, dst3.base(), len);
  BinSeq tr_bin(len, buf3);

  outCdr.reset();
  outCdr << actor.contentSubscriptionProfile.exprParams;
  ACE_Message_Block dst4;
  ACE_CDR::consolidate(&dst4, outCdr.begin());
  len = dst4.length();
  char* buf4 = new (std::nothrow) char[len];
  if (buf4 == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( subscription) allocation failed.\n"));
    return;
  }
  ArrDelAdapter<char> guard4(buf4);

  ACE_OS::memcpy(buf4, dst4.base(), len);
  ContentSubscriptionBin csp_bin;
  csp_bin.filterClassName = actor.contentSubscriptionProfile.filterClassName;
  csp_bin.filterExpr = actor.contentSubscriptionProfile.filterExpr;
  csp_bin.exprParams = std::make_pair(len, buf4);

  DActor actor_data(actor.domainId, actor.actorId, actor.topicId
                    , actor.participantId
                    , DataReader, actor.callback.c_str(), pubsub_qos
                    , dwdr_qos, tr_bin, csp_bin);

  // allocate memory for ActorData
  void* buffer;
  ACE_ALLOCATOR(buffer, allocator_->malloc
                (sizeof(PersistenceUpdater::RWActor)));

  // Initialize ActorData
  PersistenceUpdater::RWActor* persistent_data =
    new(buffer) PersistenceUpdater::RWActor(actor_data
                                            , allocator_);

  IdType_ExtId ext(actor.actorId);

  // bind ActorData with the actorId
  if (actor_index_->bind(ext, persistent_data, allocator_) != 0) {
    allocator_->free((void *) buffer);
    return;
  }
}

void
PersistenceUpdater::create(const UWActor& actor)
{
  TAO_OutputCDR outCdr;
  outCdr << actor.pubsubQos;
  ACE_Message_Block dst;
  ACE_CDR::consolidate(&dst, outCdr.begin());

  size_t len = dst.length();
  char *buf = new (std::nothrow) char[len];

  if (buf == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( publication): allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard(buf);

  ACE_OS::memcpy(buf, dst.base(), len);

  BinSeq pubsub_qos_bin(len, buf);
  QosSeq pubsub_qos(PublisherQos, pubsub_qos_bin);

  outCdr.reset();
  outCdr << actor.drdwQos;
  ACE_Message_Block dst2;
  ACE_CDR::consolidate(&dst2, outCdr.begin());

  len = dst2.length();
  char *buf2 = new (std::nothrow) char[len];

  if (buf2 == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( publication): allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard2(buf2);

  ACE_OS::memcpy(buf2, dst2.base(), len);

  BinSeq dwdr_qos_bin(len, buf2);
  QosSeq dwdr_qos(DataWriterQos, dwdr_qos_bin);

  outCdr.reset();
  outCdr << actor.transportInterfaceInfo;
  ACE_Message_Block dst3;
  ACE_CDR::consolidate(&dst3, outCdr.begin());

  len = dst3.length();
  char *buf3 = new (std::nothrow) char[len];

  if (buf3 == 0) {
    ACE_ERROR((LM_ERROR, "PersistenceUpdater::create( publication): allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard3(buf3);

  ACE_OS::memcpy(buf3, dst3.base(), len);
  BinSeq tr_bin(len, buf3);

  DActor actor_data(actor.domainId, actor.actorId, actor.topicId
                    , actor.participantId
                    , DataWriter, actor.callback.c_str(), pubsub_qos
                    , dwdr_qos, tr_bin, ContentSubscriptionBin());

  // allocate memory for ActorData
  void* buffer;
  ACE_ALLOCATOR(buffer, allocator_->malloc
                (sizeof(PersistenceUpdater::RWActor)));

  // Initialize ActorData
  PersistenceUpdater::RWActor* persistent_data =
    new(buffer) PersistenceUpdater::RWActor(actor_data
                                            , allocator_);

  IdType_ExtId ext(actor.actorId);

  // bind ActorData with the actorId
  if (actor_index_->bind(ext, persistent_data, allocator_) != 0) {
    allocator_->free((void *) buffer);
    return;
  }
}

void
PersistenceUpdater::create(const OwnershipData& /* data */)
{
  /* This method intentionally left unimplemented. */
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::DomainParticipantQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::Participant* part_data = 0;

  if (this->participant_index_->find(ext, part_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, part_data->participantQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update: ")
               ACE_TEXT("participant %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::TopicQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::Topic* topic_data = 0;

  if (this->topic_index_->find(ext, topic_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, topic_data->topicQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update: ")
               ACE_TEXT("topic %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::DataWriterQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::RWActor* actor_data = 0;

  if (this->actor_index_->find(ext, actor_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, actor_data->drdwQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update(writerQos): ")
               ACE_TEXT("publication %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::PublisherQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::RWActor* actor_data = 0;

  if (this->actor_index_->find(ext, actor_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, actor_data->pubsubQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update(publisherQos): ")
               ACE_TEXT("publication %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::DataReaderQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::RWActor* actor_data = 0;

  if (this->actor_index_->find(ext, actor_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, actor_data->drdwQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update(readerQos): ")
               ACE_TEXT("subscription %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::SubscriberQos& qos)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::RWActor* actor_data = 0;

  if (this->actor_index_->find(ext, actor_data, this->allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << qos;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    this->storeUpdate(dst, actor_data->pubsubQos.second);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update(subscriberQos): ")
               ACE_TEXT("subscription %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::update(const IdPath& id, const DDS::StringSeq& exprParams)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::RWActor* actor_data = 0;

  if (actor_index_->find(ext, actor_data, allocator_) ==  0) {
    TAO_OutputCDR outCdr;
    outCdr << exprParams;
    ACE_Message_Block dst;
    ACE_CDR::consolidate(&dst, outCdr.begin());

    storeUpdate(dst, actor_data->contentSubscriptionProfile.exprParams);

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PersistenceUpdater::update(readerQos): ")
               ACE_TEXT("subscription %C not found\n"),
               std::string(converter).c_str()));
  }
}

void
PersistenceUpdater::destroy(const IdPath& id, ItemType type, ActorType)
{
  IdType_ExtId ext(id.id);
  PersistenceUpdater::Topic* topic = 0;
  PersistenceUpdater::Participant* participant = 0;
  PersistenceUpdater::RWActor* actor = 0;

  switch (type) {
  case Update::Topic:

    if (topic_index_->unbind(ext, topic, allocator_) == 0) {
      topic->cleanup(allocator_);
      allocator_->free((void *) topic);
    }

    break;
  case Update::Participant:

    if (participant_index_->unbind(ext, participant, allocator_) == 0) {
      participant->cleanup(allocator_);
      allocator_->free((void *) participant);
    }

    break;
  case Update::Actor:

    if (actor_index_->unbind(ext, actor, allocator_) == 0) {
      actor->cleanup(allocator_);
      allocator_->free((void *) actor);
    }

    break;
  default: {
    OpenDDS::DCPS::RepoIdConverter converter(id.id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P | %t) PersistenceUpdater::destroy: ")
               ACE_TEXT("unknown entity - %C.\n"),
               std::string(converter).c_str()));
  }
  }
}

void
PersistenceUpdater::storeUpdate(const ACE_Message_Block& data, BinSeq& storage)
{
  size_t len = data.length();

  void* buffer;
  ACE_ALLOCATOR(buffer, this->allocator_->malloc(len));
  ACE_OS::memcpy(buffer, data.base(), len);

  storage.first  = len;
  storage.second = static_cast<char*>(buffer);
}

} // namespace Update

int
PersistenceUpdaterSvc_Loader::init()
{
  return ACE_Service_Config::process_directive
         (ace_svc_desc_PersistenceUpdaterSvc);
  return 0;
}

// from the "ACE Programmers Guide (P. 424)
ACE_FACTORY_DEFINE(ACE_Local_Service, PersistenceUpdaterSvc)

ACE_STATIC_SVC_DEFINE(PersistenceUpdaterSvc,
                      ACE_TEXT("PersistenceUpdaterSvc"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME(PersistenceUpdaterSvc),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

ACE_STATIC_SVC_REQUIRE(PersistenceUpdaterSvc)

OPENDDS_END_VERSIONED_NAMESPACE_DECL
