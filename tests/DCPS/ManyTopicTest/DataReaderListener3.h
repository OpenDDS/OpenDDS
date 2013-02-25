// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL3
#define DATAREADER_LISTENER_IMPL3

#include "DataReaderListener.h"

class DataReaderListenerImpl3 : public DataReaderListenerImpl
{
public:
  explicit DataReaderListenerImpl3(int num_ops_per_thread)
    : DataReaderListenerImpl(num_ops_per_thread)
  {}

  virtual void read(::DDS::DataReader_ptr reader);
};
#endif

