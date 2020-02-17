#pragma once

#include "DataWriter.h"

namespace Builder {

class DataWriterListener : public DDS::DataWriterListener {
public:
  virtual ~DataWriterListener() {}
  virtual void set_datawriter(DataWriter& datawriter) = 0;
  virtual void unset_datawriter(DataWriter& datawriter) = 0;
};

}

