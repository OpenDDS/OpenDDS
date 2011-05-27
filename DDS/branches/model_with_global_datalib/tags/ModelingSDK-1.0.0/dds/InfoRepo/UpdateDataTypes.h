/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATE_DATA_TYPES
#define UPDATE_DATA_TYPES

#include "dds/DdsDcpsInfoUtilsC.h"

#include <vector>
#include <string>

namespace Update {

enum ItemType  { Topic, Participant, Actor };
enum ActorType { DataReader, DataWriter };
enum SpecificQos {
  NoQos,
  ParticipantQos,
  TopicQos,
  DataWriterQos,
  PublisherQos,
  DataReaderQos,
  SubscriberQos
};

typedef long                            DomainIdType;
typedef OpenDDS::DCPS::RepoId           IdType; // Federation scope identifier type.
typedef std::pair <size_t, char*>       BinSeq;
typedef std::pair <SpecificQos, BinSeq> QosSeq;
typedef BinSeq                          TransportInterfaceInfo;

struct IdPath {
  DomainIdType domain;
  IdType       participant;
  IdType       id;

  IdPath(DomainIdType d, IdType p, IdType i)
  : domain(d),
      participant(p),
      id(i) { }
};

struct OwnershipData {
  DomainIdType domain;
  IdType       participant;
  long         owner;

  OwnershipData(DomainIdType d, IdType p, long o)
  : domain(d),
      participant(p),
      owner(o) { }
};

template <typename Q, typename S>
struct TopicStrt {
  DomainIdType domainId;
  IdType       topicId;
  IdType       participantId;
  S            name;
  S            dataType;
  Q            topicQos;

  TopicStrt(
    DomainIdType dom,
    IdType       to,
    IdType       pa,
    const char*  na,
    const char*  da,
    Q            tQos) : domainId(dom),
      topicId(to),
      participantId(pa),
      name(na),
      dataType(da),
      topicQos(tQos) { };
};
typedef struct TopicStrt<DDS::TopicQos&, std::string> UTopic;
typedef struct TopicStrt<QosSeq, std::string>         DTopic;

template <typename Q>
struct ParticipantStrt {
  DomainIdType domainId;
  long         owner;
  IdType       participantId;
  Q            participantQos;

  ParticipantStrt(
    DomainIdType dom,
    long         own,
    IdType       part,
    Q            pQos) : domainId(dom),
      owner(own),
      participantId(part),
      participantQos(pQos) { };
};
typedef struct ParticipantStrt<DDS::DomainParticipantQos&> UParticipant;
typedef struct ParticipantStrt<QosSeq>                     DParticipant;

template <typename PSQ, typename RWQ, typename C, typename T>
struct ActorStrt {
  DomainIdType domainId;
  IdType       actorId;
  IdType       topicId;
  IdType       participantId;
  ActorType    type;
  C            callback;
  PSQ          pubsubQos;
  RWQ          drdwQos;
  T            transportInterfaceInfo;

  ActorStrt(
    DomainIdType dom,
    IdType       act,
    IdType       top,
    IdType       part,
    ActorType    typ,
    const char*  call,
    PSQ          pub,
    RWQ          drdw,
    T            trans)
    : domainId(dom),
      actorId(act),
      topicId(top),
      participantId(part),
      type(typ),
      callback(call),
      pubsubQos(pub),
      drdwQos(drdw),
      transportInterfaceInfo(trans) { };
};
typedef struct ActorStrt<
      DDS::SubscriberQos& ,
      DDS::DataReaderQos&,
      std::string,
      OpenDDS::DCPS::TransportInterfaceInfo&> URActor;
typedef struct ActorStrt<
      DDS::PublisherQos& ,
      DDS::DataWriterQos& ,
      std::string,
      OpenDDS::DCPS::TransportInterfaceInfo&> UWActor;
typedef struct ActorStrt<QosSeq, QosSeq, std::string, BinSeq> DActor;

template <typename T, typename P, typename A, typename W>
struct ImageData {
  typedef std::vector<T> TopicSeq;
  typedef std::vector<P> ParticipantSeq;
  typedef std::vector<A> ReaderSeq;
  typedef std::vector<W> WriterSeq;

  unsigned long  sequenceNumber;
  TopicSeq       topics;
  ParticipantSeq participants;
  ReaderSeq      actors;
  WriterSeq      wActors;
};
typedef struct ImageData<UTopic*, UParticipant*, URActor*, UWActor*> UImage;
typedef struct ImageData<DTopic,  DParticipant,  DActor,   DActor>   DImage;

} // namespace Update

#endif /* UPDATE_DATA_TYPES */
