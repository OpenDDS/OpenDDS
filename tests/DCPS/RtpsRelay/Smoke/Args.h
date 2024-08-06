/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPSRELAY_SMOKE_TEST_ARGS_H
#define RTPSRELAY_SMOKE_TEST_ARGS_H

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <cstdlib>

struct Args {
  bool check_lease_recovery = false;
  bool expect_unmatch = false;
  ACE_TCHAR* override_partition = 0;
  int participant_bit_expected_instances = -1;

  int parse(int argc, ACE_TCHAR* argv[]);
};

inline int
Args::parse(int argc, ACE_TCHAR* argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("lep:b:"));

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
    case '?':
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("usage: %s [-le] [-p partition]\n"), argv[0]), EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}

#endif
