// -*- C++ -*-
//
#ifndef ENTITYPROFILES_H
#define ENTITYPROFILES_H

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model {

enum QosMaskBits {
  SetDeadlineQos                            = 0x00000001,
  SetDestinationOrderQos                    = 0x00000002,
  SetDurabilityQos                          = 0x00000004,
  SetDurabilityServiceDurationQos           = 0x00000008,
  SetDurabilityServiceHistoryDepthQos       = 0x00000010,
  SetDurabilityServiceHistoryKindQos        = 0x00000020,
  SetDurabilityServiceInstancesQos          = 0x00000040,
  SetDurabilityServiceSamplesPerInstanceQos = 0x00000080,
  SetDurabilityServiceSamplesQos            = 0x00000100,
  SetHistoryDepthQos                        = 0x00000200,
  SetHistoryKindQos                         = 0x00000400,
  SetLatencyBudgetQos                       = 0x00000800,
  SetLifespanQos                            = 0x00001000,
  SetLivelinessDurationQos                  = 0x00002000,
  SetLivelinessKindQos                      = 0x00004000,
  SetOwnershipKindQos                       = 0x00008000,
  SetOwnershipStrengthQos                   = 0x00010000,
  SetReaderDataLifecycleQos                 = 0x00020000,
  SetReliabilityKindQos                     = 0x00040000,
  SetReliabilityMaxBlockingQos              = 0x00080000,
  SetResourceMaxInstancesQos                = 0x00100000,
  SetResourceMaxSamplesPerInstanceQos       = 0x00200000,
  SetResourceMaxSamplesQos                  = 0x00400000,
  SetTimeBasedFilterQos                     = 0x00800000,
  SetTransportPriorityQos                   = 0x01000000,
  SetUserDataQos                            = 0x02000000,
  SetWriterDataLifecycleQos                 = 0x04000000
};

/**
 * [participant/<name>]
 *   # Participant Qos Policy values
 *   DomainId = <number>
 */
struct ParticipantProfile  {
  int domainId;
  DDS::DomainParticipantQos
      qos;
};

/**
 * [topic/<name>]
 *   # Topic Qos Policy values
 *   Participant = <string> # One of participant <name>
 *   Type        = <string> # Name for a registered datatype.
 */
struct TopicProfile {
  OPENDDS_STRING   participant;
  OPENDDS_STRING   type;
  DDS::TopicQos qos;
};

/**
 * [publisher/<name>]
 *   # Publisher Qos Policy values
 *   Participant    = <string> # One of participant <name>
 *   TransportIndex = <number> # Index into transport configurations
 */
struct PublisherProfile {
  OPENDDS_STRING       participant;
  unsigned int      transport;
  DDS::PublisherQos qos;
};

/**
 * [writer/<name>]
 *   # DataWriter Qos Policy values
 *   Publisher         = <string> # One of publisher <name>
 *   Topic             = <string> # One of topic <name>
 */
struct WriterProfile {
  OPENDDS_STRING        publisher;
  OPENDDS_STRING        topic;
  DDS::DataWriterQos qos;
  unsigned int       mask;

  void copyToWriterQos( ::DDS::DataWriterQos& qos);
};

/**
 * [subscriber/<name>]
 *   # Subscriber Qos Policy values
 *   Participant    = <string> # One of participant <name>
 *   TransportIndex          = <number> # Index into transport configurations
 */
struct SubscriberProfile {
  OPENDDS_STRING        participant;
  unsigned int       transport;
  DDS::SubscriberQos qos;
};

/**
 * [reader/<name>]
 *   # DataReader Qos Policy values
 *   Subscriber              = <string> # One of subscriber <name>
 *   Topic                   = <string> # One of topic <name>
 */
struct ReaderProfile {
  OPENDDS_STRING        subscriber;
  OPENDDS_STRING        topic;
  DDS::DataReaderQos qos;
  unsigned int       mask;

  void copyToReaderQos( ::DDS::DataReaderQos& qos);
};

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ENTITYPROFILES_H

