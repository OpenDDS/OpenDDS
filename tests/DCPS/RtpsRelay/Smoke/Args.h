/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPSRELAY_SMOKE_TEST_ARGS_H
#define RTPSRELAY_SMOKE_TEST_ARGS_H

#include <dds/DCPS/GuardCondition.h>

#include <ace/Argv_Type_Converter.h>
#include <ace/Event_Handler.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/Sig_Handler.h>

#include <iostream>
#include <cstdlib>

struct Args {
  bool check_lease_recovery = false;
  bool expect_unmatch = false;
  ACE_TCHAR* override_partition = 0;
  int participant_bit_expected_instances = -1;
  bool stress_test = false;
  bool terminate_on_data = false;
  bool drain_test = false;

  int parse(int argc, ACE_TCHAR* argv[]);
};

inline int
Args::parse(int argc, ACE_TCHAR* argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("lep:b:sd"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'l':
      check_lease_recovery = true;
      break;
    case 'e':
      expect_unmatch = true;
      break;
    case 'p':
      override_partition = get_opts.opt_arg();
      break;
    case 'b':
      participant_bit_expected_instances = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 's':
      stress_test = true;
      break;
    case 'd':
      drain_test = true;
      break;
    case '?':
      ACE_ERROR_RETURN((LM_ERROR, "usage: %s [-lesd] [-p partition] [-b particpant_bit_expected_instances]\n", argv[0]), EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}

struct ShutdownHandler : ACE_Event_Handler
{
  ShutdownHandler()
  {
    guard_ = new DDS::GuardCondition;
    handler_.register_handler(SIGTERM, this);
    handler_.register_handler(SIGINT, this);
  }

  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    guard_->set_trigger_value(true);
    return 0;
  }

  DDS::GuardCondition_var guard_;
  ACE_Sig_Handler handler_;
};

#endif
