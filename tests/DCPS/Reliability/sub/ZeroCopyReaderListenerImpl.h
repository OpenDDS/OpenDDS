/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYREADER_LISTENER_IMPL_H
#define ZEROCOPYREADER_LISTENER_IMPL_H

#include "DataReaderListenerImpl.h"

class ZeroCopyReaderListenerImpl : public DataReaderListenerImpl
{
public:
  ZeroCopyReaderListenerImpl(DistributedConditionSet_rch dcs);

  virtual void take_samples(Reliability::MessageDataReader_var reader_i);

private:
  Reliability::MessageSeq messages_;
  DDS::SampleInfoSeq infos_;
};

#endif /* ZEROCOPYREADER_LISTENER_IMPL_H */
