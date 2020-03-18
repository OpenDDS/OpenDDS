#include "dds/DCPS/GuardCondition.h"
#include "dds/DCPS/WaitSet.h"

#include "ace/Task.h"
#include "ace/OS_NS_unistd.h"

class External_Trigger : public ACE_Task_Base
{
public:
  explicit External_Trigger(DDS::GuardCondition_ptr gc)
    : gc_(DDS::GuardCondition::_duplicate(gc))
  {}

  int svc()
  {
    ACE_OS::sleep(1);
    gc_->set_trigger_value(true);
    return 0;
  }

private:
  DDS::GuardCondition_var gc_;
};


class Waiter : public ACE_Task_Base
{
public:
  explicit Waiter(DDS::WaitSet_ptr ws)
    : ws_(DDS::WaitSet::_duplicate(ws))
    , result_()
  {}

  int svc()
  {
    DDS::Duration_t three = {3, 0};
    DDS::ConditionSeq active;
    result_ = ws_->wait(active, three);
    return 0;
  }

  int result() { return result_; }

private:
  DDS::WaitSet_var ws_;
  int result_;
};


int ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    using namespace DDS;
    WaitSet_var ws = new WaitSet;
    GuardCondition_var gc = new GuardCondition;
    ReturnCode_t ret = ws->attach_condition(gc);
    if (ret != RETCODE_OK) return ret;

    // 0. test basic interface interactions from client code
    {
      ConditionSeq conditions;
      ws->get_conditions(conditions);
      if (conditions.length() != 1) return RETCODE_ERROR;
      GuardCondition_var gc2 = GuardCondition::_narrow(conditions[0]);
      if (gc2 != gc) return RETCODE_ERROR;
    }

    // 1. Wait shouldn't block if the condition is already triggered
    ret = gc->set_trigger_value(true);
    if (ret != RETCODE_OK) return ret;
    ConditionSeq active;
    Duration_t infinite = { DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC };
    ret = ws->wait(active, infinite);
    if (ret != RETCODE_OK) return ret;
    if (active.length() != 1 || active[0] != gc) return 1;

    // 2. Wait blocks until a 2nd thread triggers the condition
    ret = gc->set_trigger_value(false);
    if (ret != RETCODE_OK) return ret;
    External_Trigger et(gc);
    et.activate();
    ret = ws->wait(active, infinite);
    et.wait();
    if (ret != RETCODE_OK) return ret;
    if (active.length() != 1 || active[0] != gc) return 1;

    // 3. Wait on a empty WS blocks (for specified timeout)
    ret = ws->detach_condition(gc);
    if (ret != RETCODE_OK) return ret;
    Duration_t three = { 3, 0 };
    ret = ws->wait(active, three);
    if (ret != RETCODE_TIMEOUT) return 1;
    if (active.length() != 0) return 1;

    // 4. No concurrent wait on the same WS
    Waiter w(ws);
    w.activate();
    ret = ws->wait(active, three);
    w.wait();
    if (ret != RETCODE_PRECONDITION_NOT_MET
      && w.result() != RETCODE_PRECONDITION_NOT_MET)
    {
      return 1;
    }

    // 5. "OR" behavior with multiple GuardConditions
    ret = ws->attach_condition(gc);
    if (ret != RETCODE_OK) return ret;
    ret = gc->set_trigger_value(false);
    if (ret != RETCODE_OK) return ret;
    GuardCondition_var gc2 = new GuardCondition;
    ret = ws->attach_condition(gc2);
    if (ret != RETCODE_OK) return ret;
    External_Trigger et2(gc2);
    et2.activate();
    ret = ws->wait(active, infinite);
    et2.wait();
    if (ret != RETCODE_OK) return ret;
    if (active.length() != 1 || active[0] != gc2) return 1;

    // 6. Condition can be attached after a thread starts waiting
    ConditionSeq attached;
    ret = ws->get_conditions(attached);
    if (ret != RETCODE_OK) return ret;
    if (attached.length() != 2) return 1;
    ret = ws->detach_condition(gc2);
    if (ret != RETCODE_OK) return ret;
    if (!gc2->get_trigger_value()) return 1;
    ret = ws->get_conditions(attached);
    if (ret != RETCODE_OK) return ret;
    if (attached.length() != 1 || attached[0] != gc) return 1;
    Waiter w2(ws);
    w2.activate();
    ACE_OS::sleep(1);
    ret = ws->attach_condition(gc2);
    if (ret != RETCODE_OK) return ret;
    w2.wait();
    if (w2.result() != RETCODE_OK) return w2.result();

    // 7. Attaching an already-attached condition has no effect
    ret = ws->get_conditions(attached);
    if (ret != RETCODE_OK) return ret;
    CORBA::ULong n_conditions = attached.length();
    ret = ws->attach_condition(gc);
    if (ret != RETCODE_OK) return ret;
    ret = ws->get_conditions(attached);
    if (ret != RETCODE_OK) return ret;
    if (n_conditions != attached.length()) return 1;

    // 8. Detaching a non-attached condition is an error
    WaitSet_var ws2 = new WaitSet;
    ret = ws2->detach_condition(gc);
    if (ret != RETCODE_PRECONDITION_NOT_MET) return 1;

    // cleanup
    ws->detach_condition(gc);
    ws->detach_condition(gc2);
  }
  catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in GuardConditionTest.cpp:");
    return 1;
  }
  return 0;
}
