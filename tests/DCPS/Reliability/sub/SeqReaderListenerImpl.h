/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATASEQREADER_LISTENER_IMPL_H
#define DATASEQREADER_LISTENER_IMPL_H

#include "DataReaderListenerImpl.h"

using OpenDDS::Model::NullReaderListener;

class SeqReaderListenerImpl : public DataReaderListenerImpl
{
public:
  SeqReaderListenerImpl();

protected:
  virtual void take_samples(
    Reliability::MessageDataReader_var reader_i
  );
private:
  Reliability::MessageSeq messages_;
  DDS::SampleInfoSeq infos_;
};

#endif /* DATASEQREADER_LISTENER_IMPL_H */
