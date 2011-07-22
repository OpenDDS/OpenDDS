/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef READER_H
#define READER_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsDataReaderRemoteC.h"

class Factory;

class Reader
{
public:
  Reader(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataReaderListener_ptr listener);
  Reader(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::Subscriber_ptr subscriber, DDS::DataReaderListener_ptr listener);

  virtual ~Reader();

  const DDS::DomainParticipantFactory_var dpf;
  const DDS::DomainParticipant_var dp;
  const DDS::Topic_var topic;
  const DDS::Subscriber_var sub;
  const DDS::DataReader_var reader_;
};

#endif