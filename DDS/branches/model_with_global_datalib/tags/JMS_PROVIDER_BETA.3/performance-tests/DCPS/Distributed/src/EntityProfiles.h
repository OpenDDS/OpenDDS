// -*- C++ -*-
//
// $Id$
#ifndef ENTITYPROFILES_H
#define ENTITYPROFILES_H

#include "Gaussian.h"
#include "Exponential.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataCollector_T.h"
#include <string>

namespace Test {

/**
 * [participant/<name>]
 *   # Participant Qos Policy values
 *   # Test execution parameters
 *   DomainId = <number>
 */
struct ParticipantProfile  {
  int domainId;
  ::DDS::DomainParticipantQos
      qos;
};

/**
 * [topic/<name>]
 *   # Topic Qos Policy values
 */
struct TopicProfile {
  ::DDS::TopicQos qos;
};

/**
 * [publication/<name>]
 *   # Publisher Qos Policy values
 *   # DataWriter Qos Policy values
 *   # Test execution parameters
 *   Participant      = <string> # One of participant <name>
 *   Topic            = <string> # One of topic <name>
 *   TransportIndex   = <number> # Index into transport configurations
 *   MessageSource    = <string> # One of subscription <name>
 *   MessageRate      = <number> # Samples per second
 *   MessageSize      = <number> # bytes per sample
 *   MessageMax       = <number> # upper bound for size
 *   MessageMin       = <number> # lower bound for size
 *   MessageDeviation = <number> # standard deviation for size
 */
struct PublicationProfile {
  std::string          participant;
  std::string          topic;
  unsigned int         transport;
  std::string          source;
  Gaussian             size;
  Exponential          rate;
  ::DDS::PublisherQos  publisherQos;
  ::DDS::DataWriterQos writerQos;
};

/**
 * [subscription/<name>]
 *   # Subscriber Qos Policy values
 *   # DataReader Qos Policy values
 *   # Test execution parameters
 *   Participant                         = <string> # One of participant <name>
 *   Topic                               = <string> # One of topic <name>
 *   TransportIndex                      = <number> # Index into transport configurations
 *   DataCollectionFile                  = <string> # Filename for collected data
 *   DataCollectionBound                 = <number>
 *   DataCollectionRetention             = <string> # One of ALL, OLDEST, NEWEST
 */
struct SubscriptionProfile {
  std::string          participant;
  std::string          topic;
  unsigned int         transport;
  std::string          datafile;
  int                  bound;
  ::OpenDDS::DCPS::DataCollector<double>::OnFull
                       retention;
  ::DDS::SubscriberQos subscriberQos;
  ::DDS::DataReaderQos readerQos;
};

} // End of namespace Test

#endif // ENTITYPROFILES_H

