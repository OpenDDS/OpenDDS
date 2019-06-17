// -*- C++ -*-
//
#ifndef ENTITYPROFILES_H
#define ENTITYPROFILES_H

#include "StatisticalValue.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataCollector_T.h"
#include <string>

namespace Test {

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
 *   # Test execution parameters
 *   DomainId = <number>
 */
struct ParticipantProfile  {
  int domainId;
  DDS::DomainParticipantQos qos;
};

/**
 * [topic/<name>]
 *   # Topic Qos Policy values
 *   Participant = <string> # One of participant <name>
 */
struct TopicProfile {
  std::string   participant;
  DDS::TopicQos qos;
};

/**
 * [publication/<name>]
 *   # Publisher Qos Policy values
 *   # DataWriter Qos Policy values
 *   # Test execution parameters
 *   Topic             = <string> # One of topic <name>
 *   TransportConfig   = <string> # Name of transport config
 *   Associations      = <number> # Number of subscriptions to match before starting.
 *   StartAfterDelay   = <number> # Delay before writes start after matching.
 *   MessageSource     = <string> # One of subscription <name>
 *   MessageRateType   = <string> # One of FIXED, POISSON
 *   MessageRate       = <number> # Samples per second, Poisson arrival times
 *   MessageSizeType   = <string> # One of FIXED, UNIFORM, GAUSSIAN
 *   MessageSize       = <number> # bytes per sample
 *   MessageMax        = <number> # upper bound for size
 *   MessageMin        = <number> # lower bound for size
 *   MessageDeviation  = <number> # standard deviation for size
 *   InstanceType      = <string> # One of FIXED, UNIFORM, GAUSSIAN
 *   InstanceMean      = <number> # average value of instance key for sending
 *   InstanceMax       = <number> # upper bound for number of instances
 *   InstanceMin       = <number> # lower bound for number of instances
 *   InstanceDeviation = <number> # standard deviation of instance key for sending
 *   AckDelay          = <number> # >0 passed to wait_for_acks()
 */
struct PublicationProfile {
  std::string               topic;
  std::string               transportConfig;
  std::string               source;
  StatisticalValue<long>*   instances;
  StatisticalValue<long>*   size;
  StatisticalValue<double>* rate;
  DDS::PublisherQos         publisherQos;
  DDS::DataWriterQos        writerQos;
  unsigned int              writerQosMask;
  unsigned int              associations;
  unsigned int              delay;
  unsigned int              ackDelay;

  void copyToWriterQos( ::DDS::DataWriterQos& qos);
};

/**
 * [subscription/<name>]
 *   # Subscriber Qos Policy values
 *   # DataReader Qos Policy values
 *   # Test execution parameters
 *   Topic                   = <string> # One of topic <name>
 *   TransportConfig         = <string> # Name of transport config
 *   DataCollectionFile      = <string> # Filename for collected data
 *   DataCollectionBound     = <number>
 *   DataCollectionRetention = <string> # One of ALL, OLDEST, NEWEST
 */
struct SubscriptionProfile {
  std::string        topic;
  std::string        transportConfig;
  std::string        datafile;
  int                bound;
  OpenDDS::DCPS::DataCollector<double>::OnFull
                     retention;
  DDS::SubscriberQos subscriberQos;
  DDS::DataReaderQos readerQos;
  unsigned int       readerQosMask;

  void copyToReaderQos( ::DDS::DataReaderQos& qos);
};

} // End of namespace Test

#endif // ENTITYPROFILES_H

