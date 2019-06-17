#pragma once

#include "Publisher.h"

namespace Builder {

class PublisherListener : public DDS::PublisherListener {
public:
  virtual void set_publisher(Publisher& publisher) = 0;
};

}

