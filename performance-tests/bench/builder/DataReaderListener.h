#pragma once

#include "DataReader.h"

namespace Builder {

class DataReaderListener : public DDS::DataReaderListener {
public:
  virtual void set_datareader(DataReader& datareader) = 0;
};

}

