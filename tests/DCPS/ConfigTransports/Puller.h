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

class Puller
{
public:
  Puller(const Factory& f,
         const DDS::DomainParticipantFactory_var& factory,
         const DDS::DomainParticipant_var& participant,
         const DDS::DataReaderListener_var& listener);

  Puller(const Factory& f,
         const DDS::DomainParticipantFactory_var& factory,
         const DDS::DomainParticipant_var& participant,
         const DDS::Subscriber_var& subscriber,
         const DDS::DataReaderListener_var& listener);

  virtual ~Puller();
  int pull(const ACE_Time_Value& duration);


  const DDS::DomainParticipantFactory_var dpf;
  const DDS::DomainParticipant_var dp;
  const DDS::Topic_var topic;
  const DDS::Subscriber_var sub;
  const DDS::DataReader_var reader_;
};

#endif
