#pragma once

#include "ace/Asynch_IO.h"

namespace Bench {

template <typename T>
class MemFunHandler : public ACE_Handler {
public:
  MemFunHandler(void (T::* fun)(void), T& obj) : fun_(fun), obj_(obj) {}
  virtual ~MemFunHandler() {}
  MemFunHandler(const MemFunHandler&) = delete;

  void handle_time_out(const ACE_Time_Value& /*tv*/, const void* /*act*/) override {
    (obj_.*fun_)();
  }

protected:
  void (T::* fun_)(void);
  T& obj_;
};

}

