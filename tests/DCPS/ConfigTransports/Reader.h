/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef READER_H
#define READER_H

#include "common.h"

class Reader
{
public:
  Reader(DDS::DomainParticipantFactory_ptr factory, DDS::DataReaderListener_ptr listener);
  Reader(DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataReaderListener_ptr listener);

  virtual ~Reader();

  bool verify_transport();


private:
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var dp;
  DDS::Topic_var topic;
  DDS::Subscriber_var sub;
  DDS::DataReader_var dr;
};

#endif