/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include <ace/Task.h>
#include <dds/DdsDcpsPublicationC.h>

class Writer {
public:

  Writer(DDS::DataWriter_ptr writer1, DDS::DataWriter_ptr writer2);

  void write(bool reliable, int num_msgs);

  int get_timeout_writes() const;

private:
  DDS::DataWriter_var writer1_;
  DDS::DataWriter_var writer2_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
};

#endif /* WRITER_H */
