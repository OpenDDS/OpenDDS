/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsDataWriterRemoteC.h"

class Factory;

class Writer
{
public:
  Writer(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataWriterListener_ptr listener);
  Writer(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::Publisher_ptr publisher, DDS::DataWriterListener_ptr listener);
  virtual ~Writer();

  const DDS::DomainParticipantFactory_var dpf;
  const DDS::DomainParticipant_var dp;
  const DDS::Publisher_var pub;
  const DDS::Topic_var topic;
  const DDS::DataWriter_var writer_;
};

#endif