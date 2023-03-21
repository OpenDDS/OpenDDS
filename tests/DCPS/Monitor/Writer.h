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

  Writer(DDS::DataWriter_ptr writer);

  void start();

  void end();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

  int get_timeout_writes() const;

private:
  DDS::DataWriter_var writer_;
  OpenDDS::DCPS::Atomic<int> finished_instances_;
  OpenDDS::DCPS::Atomic<int> timeout_writes_;
};

#endif /* WRITER_H */
