#pragma once

#include "Topic.h"

namespace Builder {

class TopicListener : public DDS::TopicListener {
public:
  virtual void set_topic(Topic& topic) = 0;
};

}

