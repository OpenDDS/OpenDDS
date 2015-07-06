/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYREADER_LISTENER_IMPL_H
#define ZEROCOPYREADER_LISTENER_IMPL_H

#include "DataReaderListenerImpl.h"

using OpenDDS::Model::NullReaderListener;

class ZeroCopyReaderListenerImpl : public DataReaderListenerImpl
{
public:
  ZeroCopyReaderListenerImpl();

  virtual void take_samples(Reliability::MessageDataReader_var reader_i);

private:
  Reliability::MessageSeq messages_;
  DDS::SampleInfoSeq infos_;
};

#endif /* ZEROCOPYREADER_LISTENER_IMPL_H */
