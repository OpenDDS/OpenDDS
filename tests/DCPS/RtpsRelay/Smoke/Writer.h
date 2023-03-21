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

  Writer(DDS::DataWriter_ptr writer, size_t count, bool delay);

  void start();

  void end();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

private:
  DDS::DataWriter_var writer_;
  OpenDDS::DCPS::Atomic<int> finished_instances_;
  size_t count_;
  bool delay_;
};

#endif /* WRITER_H */
