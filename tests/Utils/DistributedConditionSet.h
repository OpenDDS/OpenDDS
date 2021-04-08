#pragma once

#ifndef TESTUTILS_DISTRIBUTED_CONDITION_SET_H
#define TESTUTILS_DISTRIBUTED_CONDITION_SET_H

#include <dds/DCPS/RcObject.h>

#include <ace/Condition_Thread_Mutex.h>

#include <set>

class DistributedConditionSet : public OpenDDS::DCPS::RcObject {
public:
  virtual void post(const std::string& actor, const std::string& condition) = 0;
  virtual void wait_for(const std::string& waiting_actor, const std::string& posting_actor, const std::string& condition) const = 0;
};

typedef OpenDDS::DCPS::RcHandle<DistributedConditionSet> DistributedConditionSetHandle;

class InMemoryDistributedConditionSet : public DistributedConditionSet {
public:
  InMemoryDistributedConditionSet()
    : condition_(mutex_)
  {}

  void post(const std::string& actor, const std::string& condition) {
    ACE_DEBUG((LM_INFO, "(%P|%t) %C posting %C\n", actor.c_str(), condition.c_str()));
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    set_.insert(PairType(actor, condition));
    condition_.broadcast();
  }

  void wait_for(const std::string& waiting_actor, const std::string& posting_actor, const std::string& condition) const {
    ACE_DEBUG((LM_INFO, "(%P|%t) %C waiting_for %C %C\n", waiting_actor.c_str(), posting_actor.c_str(), condition.c_str()));
    const PairType key(posting_actor, condition);

    mutex_.acquire();

    for (;;) {
      if (set_.count(key)) {
        mutex_.release();
        return;
      }
      condition_.wait();
    }
  }

private:
  typedef std::pair<std::string, std::string> PairType;
  typedef std::set<PairType> SetType;
  SetType set_;
  mutable ACE_Thread_Mutex mutex_;
  mutable ACE_Condition_Thread_Mutex condition_;
};

#endif // TESTUTILS_DISTRIBUTED_CONDITION_SET_H
