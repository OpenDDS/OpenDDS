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
#include "Options.h"

struct WriterSample
{
  DDS::DataWriter_var writer;
  Messenger::Message message;
};

class Writer {
public:

  typedef std::vector<WriterSample> Writers;
  Writer(const Options& options, Writers& writers);

  bool write();

private:
  const Options& options_;
  Writers& writers_;
};

#endif /* WRITER_H */
