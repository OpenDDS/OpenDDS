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

#include "Args.h"
class Writer : public ACE_Task_Base {
public:
  enum message_t {messenger, data};

  Writer(DDS::DataWriter_ptr writer, const Writer::message_t messageType = Writer::messenger);

  void start();

  void end();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

private:
  DDS::DataWriter_var writer_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;

  void writeMessenger();
  void writeData();
protected:
  const message_t messageType_;
};

#endif /* WRITER_H */
