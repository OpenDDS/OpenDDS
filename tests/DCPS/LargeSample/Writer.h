/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include "MessengerTypeSupportImpl.h"

#include <dds/DdsDcpsPublicationC.h>

#include <ace/Task.h>

#include <vector>

typedef std::vector<DDS::DataWriter_var> DataWriters;

class Writer {
public:
  Writer(DataWriters& datawriters, int my_pid);

  void write(bool reliable, int num_msgs, unsigned data_field_length_offset);

  int get_timeout_writes() const;

private:
  DataWriters& datawriters_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
  const int my_pid_;
};

#endif /* WRITER_H */
