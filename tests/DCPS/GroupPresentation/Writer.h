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

class Writer : public ACE_Task_Base {
public:

  Writer(DDS::Publisher_ptr publisher, DDS::DataWriter_ptr writer);

  void start();

  void end();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

  int get_timeout_writes() const;

  int next_count ();

private:
  DDS::Publisher_var publisher_;
  DDS::DataWriter_var writer_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
};

#endif /* WRITER_H */
