/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef WRITER_H
#define WRITER_H

#include "Args.h"

#include <dds/DCPS/GuardCondition.h>

#include <dds/DdsDcpsPublicationC.h>

#include <ace/Task.h>

class Writer : public ACE_Task_Base {
public:

  Writer(DDS::DataWriter_ptr writer, const Args& args);

  void start();

  void end();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

private:
  DDS::DataWriter_var writer_;
  const Args args_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  DDS::GuardCondition_var guard_condition_;
};

#endif /* WRITER_H */
