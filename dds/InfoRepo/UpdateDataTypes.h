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

typedef char* BinStr;

typedef std::pair<ssize_t, BinStr> QosType;

typedef long IdType;

typedef std::string StringType;

typedef BinStr IORType;

// Data Types:

typedef struct {
  IdType domainId;
  IdType topicId; // Unique system-wide
  IdType particiapntId;
  StringType name;
  StringType dataType;
  QosType topicQos;
} TopicData;

typedef struct {
  IdType domainId;
  IdType participantId; // Unique system-wide
  QosType participantQos;
  IORType ior;
} ParticipantData;

typedef struct {
  IdType actorId; // Unique system-wide
  IdType topicId;
  IdType participantId;
  ActorType type;
  QosType pubsubQos;
  QosType drdwQos;
  //std::vector<TransportInterfaceInfo> transportInterfaceInfo;
} ActorData;

struct ImageData {
  unsigned long sequenceNumber;
  std::vector<TopicData> topics;
  std::vector<ParticipantData> participants;
  std::vector<ActorData> actors;
};

#endif // _UPDATE_DATA_TYPES
