#pragma once

#include "StoolTypeSupportImpl.h"

namespace Stool {

class DataHandler {
public:
  virtual ~DataHandler() {}

  virtual void on_data(const Data& data) = 0;
};

}

