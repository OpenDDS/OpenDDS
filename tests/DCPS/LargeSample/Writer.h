/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include <ace/Task.h>
#include <dds/DdsDcpsPublicationC.h>
#include "MessengerTypeSupportImpl.h"

class Writer {
public:

  Writer(DDS::DataWriter_ptr writer1, DDS::DataWriter_ptr writer2, int my_pid);

  void write(bool reliable, int num_msgs);

  int get_timeout_writes() const;

  static int calc_sample_length(long sample_id, long writer_id) {
    // Writer ID is 1 or 2
    // Sample ID is 0 to 9
    // Lengths will vary from 10k to 155k
    return int((sample_id * 1.5) + writer_id) * 10 * 1024;
  }

  void extend_sample(Messenger::Message& message);
private:
  DDS::DataWriter_var writer1_;
  DDS::DataWriter_var writer2_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
  const int my_pid_;
};

#endif /* WRITER_H */
