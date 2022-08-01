/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PULLER_H
#define PULLER_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsSubscriptionC.h"

class Factory;

class Puller {
public:
  Puller(const Factory& factory,
         const DDS::DomainParticipantFactory_var& dpf,
         const DDS::DomainParticipant_var& participant,
         const DDS::DataReaderListener_var& listener);

  Puller(const Factory& factory,
         const DDS::DomainParticipantFactory_var& dpf,
         const DDS::DomainParticipant_var& participant,
         const DDS::Subscriber_var& subscriber,
         const DDS::DataReaderListener_var& listener);

  virtual ~Puller();
  int pull(const ACE_Time_Value& duration);

  const DDS::DomainParticipantFactory_var dpf_;
  const DDS::DomainParticipant_var dp_;
  const DDS::Topic_var topic_;
  const DDS::Subscriber_var sub_;
  const DDS::DataReader_var reader_;
};

#endif
