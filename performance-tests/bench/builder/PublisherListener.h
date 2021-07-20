#pragma once

#include "Publisher.h"

namespace Builder {

class PublisherListener : public DDS::PublisherListener {
public:
  virtual void set_publisher(Publisher& publisher) = 0;
  virtual void unset_publisher(Publisher& publisher) = 0;
};

}
