#pragma once

#include "Subscriber.h"

namespace Builder {

class SubscriberListener : public DDS::SubscriberListener {
public:
  virtual void set_subscriber(Subscriber& subscriber) = 0;
  virtual void unset_subscriber(Subscriber& subscriber) = 0;
};

}
