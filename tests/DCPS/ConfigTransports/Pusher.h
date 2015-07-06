/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PUSHER_H
#define PUSHER_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsPublicationC.h"

class Factory;

class Pusher
{
public:
  Pusher(const Factory& f,
         const DDS::DomainParticipantFactory_var& factory,
         const DDS::DomainParticipant_var& participant,
         const DDS::DataWriterListener_var& listener);

  Pusher(const Factory& f,
         const DDS::DomainParticipantFactory_var& factory,
         const DDS::DomainParticipant_var& participant,
         const DDS::Publisher_var& publisher,
         const DDS::DataWriterListener_var& listener);

  virtual ~Pusher();
  int push(const ACE_Time_Value& duration);

  const DDS::DomainParticipantFactory_var dpf;
  const DDS::DomainParticipant_var dp;
  const DDS::Publisher_var pub;
  const DDS::Topic_var topic;

  const DDS::DataWriter_var writer_;
};

#endif
