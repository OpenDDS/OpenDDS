#pragma once

#include "BenchTypeSupportImpl.h"

namespace Bench {

class DataHandler {
public:
  virtual ~DataHandler() {}

  virtual void on_data(const Data& data) = 0;
};

}

