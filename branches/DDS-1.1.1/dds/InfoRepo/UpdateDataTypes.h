// -*- C++ -*-

/**
 * @file      UpdateDataTypes.h
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */

#ifndef _UPDATE_DATA_TYPES
#define _UPDATE_DATA_TYPES

#include <vector>
#include <string>

// Enumerations:

enum ActorType
  {
    DataReader
    , DataWriter
  };

enum ItemType
  {
    Topic
    , Participant
    , Actor
  };

enum SpecificQos
  {
    ParticipantQos
    , TopicQos
    , DataWriterQos
    , PublisherQos
    , DataReaderQos
    , SubscriberQos
  };

// Typedefs:

typedef long IdType;

typedef std::pair <size_t, char*> BinSeq;

typedef std::pair <SpecificQos, BinSeq> QosSeq;

typedef BinSeq TransportInterfaceInfo;

// Data Types:

template <typename Q, typename S>
struct TopicStrt {
  IdType domainId;
  IdType topicId; // Unique system-wide
  IdType participantId;
  S name;
  S dataType;
  Q topicQos;

  TopicStrt (IdType dom, IdType to, IdType pa
             , const char* na, const char* da, Q tQos)
    : domainId (dom), topicId (to), participantId (pa)
    , name (na), dataType (da), topicQos (tQos)
  { };
};

template <typename Q>
struct ParticipantStrt {
  IdType domainId;
  IdType participantId; // Unique system-wide
  Q participantQos;

  ParticipantStrt (IdType dom, IdType part
                   , Q pQos)
    : domainId (dom), participantId (part)
      , participantQos (pQos)
  { };
};

template <typename PSQ, typename RWQ, typename C, typename T>
struct ActorStrt {
  IdType domainId;
  IdType actorId; // Unique system-wide
  IdType topicId;
  IdType participantId;
  ActorType type;
  C callback;
  PSQ pubsubQos;
  RWQ drdwQos;
  T transportInterfaceInfo;

  ActorStrt (IdType dom, IdType act, IdType top
             , IdType part
             , ActorType typ, const char* call
             , PSQ pub, RWQ drdw, T trans)
    : domainId (dom), actorId (act), topicId (top)
    , participantId (part)
    , type (typ), callback (call), pubsubQos (pub)
    , drdwQos (drdw), transportInterfaceInfo (trans)
  { };
};

template <typename T, typename P, typename A, typename W>
struct ImageData {
  typedef std::vector <T> TopicSeq;
  typedef std::vector <P> ParticipantSeq;
  typedef std::vector <A> ReaderSeq;
  typedef std::vector <W> WriterSeq;

  unsigned long sequenceNumber;
  TopicSeq topics;
  ParticipantSeq participants;
  ReaderSeq actors;
  WriterSeq wActors;
};

#endif // _UPDATE_DATA_TYPES
