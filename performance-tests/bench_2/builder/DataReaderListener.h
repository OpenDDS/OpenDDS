#pragma once

#include "DataReader.h"

namespace Builder {

class DataReaderListener : public DDS::DataReaderListener {
public:
  virtual ~DataReaderListener() {}
  virtual void set_datareader(DataReader& datareader) = 0;
  virtual void unset_datareader(DataReader& datareader) = 0;
};

}

