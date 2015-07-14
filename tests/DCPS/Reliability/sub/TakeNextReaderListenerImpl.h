/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAKE_NEXT_READER_LISTENER_IMPL_H
#define TAKE_NEXT_READER_LISTENER_IMPL_H

#include "DataReaderListenerImpl.h"

class TakeNextReaderListenerImpl : public DataReaderListenerImpl
{
public:
  TakeNextReaderListenerImpl();

  virtual void take_samples(Reliability::MessageDataReader_var reader_i);
};

#endif /* TAKE_NEXT_READER_LISTENER_IMPL_H */
