// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL2
#define DATAREADER_LISTENER_IMPL2

#include "DataReaderListener.h"

class DataReaderListenerImpl2 : public DataReaderListenerImpl
{
public:
  explicit DataReaderListenerImpl2(int num_ops_per_thread)
    : DataReaderListenerImpl(num_ops_per_thread)
  {}

  virtual void read(::DDS::DataReader_ptr reader);
};
#endif

