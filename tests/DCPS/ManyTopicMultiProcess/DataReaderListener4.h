// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL4
#define DATAREADER_LISTENER_IMPL4

#include "DataReaderListener.h"

class DataReaderListenerImpl4 : public DataReaderListenerImpl
{
public:
  explicit DataReaderListenerImpl4(int num_ops_per_thread)
    : DataReaderListenerImpl(num_ops_per_thread)
  {}

  virtual void read(::DDS::DataReader_ptr reader);
};
#endif

