/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include "common.h"

class Writer
{
public:
  Writer(DDS::DomainParticipantFactory_ptr factory, DDS::DataWriterListener_ptr listener);
  Writer(DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataWriterListener_ptr listener);
  virtual ~Writer();

  bool verify_transport();


private:
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var dp;
  DDS::Topic_var topic;
  DDS::Publisher_var pub;
  DDS::DataWriter_var dw;
};

#endif