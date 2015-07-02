/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <tests/Utils/DataReaderListenerImpl.h>
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

class Listener
  : public virtual TestUtils::DataReaderListenerImpl< ::Xyz::Foo, ::Xyz::FooDataReader>
{
public:
  Listener();

  unsigned long long sample_count() { return sample_count_; }
  unsigned long long expected_count() { return expected_count_; }

  protected:
  virtual void on_sample(const ::Xyz::Foo& msg);

  long sample_count_;
  long expected_count_;
  long expected_seq_;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
