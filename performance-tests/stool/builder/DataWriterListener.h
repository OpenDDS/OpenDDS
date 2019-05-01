#pragma once

#include "DataWriter.h"

namespace Builder {

class DataWriterListener : public DDS::DataWriterListener {
public:
  virtual void set_datawriter(DataWriter& datawriter) = 0;
};

}

