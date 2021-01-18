#pragma once

#ifndef TESTUTILS_WAIT_FOR_SAMPLE_H
#define TESTUTILS_WAIT_FOR_SAMPLE_H

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/WaitSet.h"

#include <iostream>

namespace Utils {

inline
bool waitForSample(const DDS::DataReader_var& dr)
{
  DDS::ReadCondition_var dr_rc = dr->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                                          DDS::ANY_VIEW_STATE,
                                                          DDS::ALIVE_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(dr_rc);
  DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq active;
  DDS::ReturnCode_t ret = ws->wait(active, infinite);
  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);
  if (ret != DDS::RETCODE_OK) {
    std::cerr << "ERROR: wait(rc) failed" << std::endl;
    return false;
  }
  return true;
}

}

#endif
