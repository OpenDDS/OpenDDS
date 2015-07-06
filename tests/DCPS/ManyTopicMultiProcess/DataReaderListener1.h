// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL1
#define DATAREADER_LISTENER_IMPL1

#include "DataReaderListener.h"

class DataReaderListenerImpl1 : public DataReaderListenerImpl
{
public:
  explicit DataReaderListenerImpl1(int num_ops_per_thread)
    : DataReaderListenerImpl(num_ops_per_thread)
  {}

  virtual void read(::DDS::DataReader_ptr reader);
};
#endif

