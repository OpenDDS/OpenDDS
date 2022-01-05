#pragma once

#ifndef TESTUTILS_DISTRIBUTED_CONDITION_SET_H
#define TESTUTILS_DISTRIBUTED_CONDITION_SET_H

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/PoolAllocator.h>

#include <ace/Condition_Thread_Mutex.h>
#include <ace/OS_NS_sys_stat.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_unistd.h>

#include <set>
#include <utility>

/**
 * DistributedConditionSet applies the concept of a condition variable
 * to coordination in integration tests.
 *
 * For various reasons, different processes, threads, etc., need to
 * coordinate in integration tests.  Some of this coordination can be
 * done with OpenDDS, e.g., waiting for a publication or subscription
 * to match, waiting for acknowledgments.  However, some coordination
 * requires an auxiliary communication channel.  For example, an
 * unreliable reader may want to tell a writer that it has received
 * all of the messages.  Using an auxiliary communication channel is
 * preferred to adding sleep statements which results in brittle
 * tests.
 *
 * The idea of a condition variable is that one or more threads in a
 * multi-threaded application are waiting on a condition.  When that
 * condition is established, the thread establishing the condition
 * signals one or more waiting threads to resume.  The condition
 * variable concept can be extended to arbitrary distributed systems
 * by allowing threads in different processes to wait and signal.
 *
 * Applying the condition variable concept to integration tests means
 * that an arbitrary actor should be allowed to wait on a condition
 * until another actor establishes that condition.  Establishing a
 * condition is called posting the condition.
 *
 * DistributedConditionSet defines an interface that applies the
 * condition variable concept to integration testing.  An actor can
 * post a condition whereby it asserts that the condition has been
 * established and an actor can wait until a condition is established.
 * For generality, actors and conditions are just unique strings.
 *
 * DistributedConditionSet can have different implementations to
 * satisfy different objectives.  For example, a single-process test
 * can use an in-memory collection of conditions.  A multi-process
 * test could use the file system, a network protocol, or even OpenDDS
 * to distribute and record conditions.
 */
class DistributedConditionSet : public OpenDDS::DCPS::RcObject
{
public:
  /// Establish a condition.
  virtual void post(const OpenDDS::DCPS::String& actor, const OpenDDS::DCPS::String& condition) = 0;
  /// Block until a condition is established.  (waiting_actor is only used for logging.)
  virtual void wait_for(const OpenDDS::DCPS::String& waiting_actor,
                        const OpenDDS::DCPS::String& posting_actor,
                        const OpenDDS::DCPS::String& condition) const = 0;
};

typedef OpenDDS::DCPS::RcHandle<DistributedConditionSet> DistributedConditionSet_rch;

class InMemoryDistributedConditionSet : public DistributedConditionSet
{
public:
  InMemoryDistributedConditionSet()
    : condition_(mutex_)
  {}

  void post(const OpenDDS::DCPS::String& actor, const OpenDDS::DCPS::String& condition)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) InMemoryDistributedConditionSet %C posting %C\n",
               actor.c_str(), condition.c_str()));
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    set_.insert(PairType(actor, condition));
    condition_.broadcast();
  }

  void wait_for(const OpenDDS::DCPS::String& waiting_actor,
                const OpenDDS::DCPS::String& posting_actor,
                const OpenDDS::DCPS::String& condition) const
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) InMemoryDistributedConditionSet %C waiting_for %C %C\n",
               waiting_actor.c_str(), posting_actor.c_str(), condition.c_str()));

    const PairType key(posting_actor, condition);

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (;;) {
      if (set_.count(key)) {
        break;
      }
      condition_.wait();
    }

    ACE_DEBUG((LM_INFO, "(%P|%t) InMemoryDistributedConditionSet %C waiting_for %C %C done\n",
               waiting_actor.c_str(), posting_actor.c_str(), condition.c_str()));
  }

private:
  typedef std::pair<OpenDDS::DCPS::String, OpenDDS::DCPS::String> PairType;
  typedef std::set<PairType> SetType;
  SetType set_;
  mutable ACE_Thread_Mutex mutex_;
  mutable ACE_Condition_Thread_Mutex condition_;
};

const char DCS_DIR[] = "DCS";

class FileBasedDistributedConditionSet : public DistributedConditionSet {
public:
  void post(const OpenDDS::DCPS::String& actor, const OpenDDS::DCPS::String& condition)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) FileBasedDistributedConditionSet %C posting %C\n",
               actor.c_str(), condition.c_str()));

    if (ACE_OS::mkdir(DCS_DIR) && errno != EEXIST) {
      ACE_ERROR((LM_ERROR, "(%P|%t) FileBasedDistributedConditionSet could not mkdir %C: %p\n",
                 DCS_DIR, ""));

    }
    const OpenDDS::DCPS::String actor_path = OpenDDS::DCPS::String(DCS_DIR) + "/" + actor;
    if (ACE_OS::mkdir(actor_path.c_str()) && errno != EEXIST) {
      ACE_ERROR((LM_ERROR, "(%P|%t) FileBasedDistributedConditionSet could not mkdir %C: %p\n",
                 actor_path.c_str(), ""));
    }
    const OpenDDS::DCPS::String condition_path = actor_path + "/" + condition;
    ACE_HANDLE fd = ACE_OS::open(condition_path.c_str(), O_WRONLY|O_CREAT);
    if (fd == ACE_INVALID_HANDLE) {
      ACE_ERROR((LM_ERROR, "(%P|%t) FileBasedDistributedConditionSet could not create %C: %p\n",
                 condition_path.c_str(), ""));
    }
    ACE_OS::close(fd);
  }

  void wait_for(const OpenDDS::DCPS::String& waiting_actor,
                const OpenDDS::DCPS::String& posting_actor,
                const OpenDDS::DCPS::String& condition) const
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) FileBasedDistributedConditionSet %C waiting_for %C %C\n",
               waiting_actor.c_str(), posting_actor.c_str(), condition.c_str()));

    const OpenDDS::DCPS::String path = OpenDDS::DCPS::String(DCS_DIR) + "/" + posting_actor + "/" + condition;
    ACE_HANDLE fd;
    for (fd = ACE_INVALID_HANDLE; fd == ACE_INVALID_HANDLE; fd = ACE_OS::open(path.c_str(), O_RDONLY)) {
      ACE_OS::sleep(1);
    }
    ACE_OS::close(fd);

    ACE_DEBUG((LM_INFO, "(%P|%t) FileBasedDistributedConditionSet %C waiting_for %C %C done\n",
               waiting_actor.c_str(), posting_actor.c_str(), condition.c_str()));
  }

};

#endif // TESTUTILS_DISTRIBUTED_CONDITION_SET_H
