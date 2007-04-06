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

#include "dds/DdsDcpsInfrastructureC.h"

#include <vector>
#include <memory>
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

enum SpecificQosType
  {
    DomainParticipantQos
    , TopicQos
    , DataWriterQos
    , PublisherQos
    , DataReaderQos
    , SubscriberQos
  };

// Typedefs:

typedef std::vector <char> BinStr;

typedef std::pair <ItemType, BinStr> QosType;

typedef long IdType;

typedef std::string StringType;

typedef BinStr IORType;

// Data Types:

template <typename Q, typename S>
struct TopicStrt {
  IdType domainId;
  IdType topicId; // Unique system-wide
  IdType particiapntId;
  S name;
  S dataType;
  Q topicQos;
};

template <typename Q>
struct ParticipantStrt {
  IdType domainId;
  IdType participantId; // Unique system-wide
  Q participantQos;
};

template <typename Q, typename B>
struct ActorStrt {
  IdType actorId; // Unique system-wide
  IdType topicId;
  IdType participantId;
  ActorType type;
  B ior;
  Q pubsubQos;
  Q drdwQos;
  //std::vector<TransportInterfaceInfo> transportInterfaceInfo; // TBD
};

typedef struct TopicStrt <QosType, StringType> TopicData;
typedef struct ParticipantStrt <QosType> ParticipantData;
typedef struct ActorStrt <QosType, IORType> ActorData;

struct ImageData {
  unsigned long sequenceNumber;
  std::vector <TopicData> topics;
  std::vector <ParticipantData> participants;
  std::vector <ActorData> actors;
};

#endif // _UPDATE_DATA_TYPES
